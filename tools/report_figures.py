"""Regenerate the report figures and the validation numbers.

Runs PotentialFlowLab.exe on the example scenes with its command-line exports, so every figure and
number in docs/report.md comes from the program itself:

    python tools/report_figures.py [--exe build/msvc/bin/Release/PotentialFlowLab.exe]

1. Figures: each example is rendered (tracer particles off, overlays as listed in FIGURES) to
   docs/figures/<name>.png.
2. Validation: the field CSV of each verification scene is compared with the classical closed-form
   solutions (Anderson, Fundamentals of Aerodynamics, ch. 3), written here in polar form,
   independently of the program's complex-potential code. A Markdown table of the errors is
   printed and written to docs/figures/validation.md.
3. docs/figures/cylinder_cp.png: Cp along the y axis above the cylinder, program vs. exact.

Needs numpy and matplotlib. The exe opens its window briefly for every export.
"""

import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
EXAMPLES = ROOT / "examples"

# name -> (example scene, plot-setting overrides)
FIGURES = {
    "uniform_45deg": ("uniform_45deg", {}),
    "source_sink_vortex": ("source_sink_vortex", {"equipotentials": True}),
    "cylinder": ("cylinder", {"speed_contours": True}),
    "lifting_cylinder": ("lifting_cylinder", {"equipotentials": True}),
    "source_near_wall": ("source_near_wall", {"show_images": True}),
    "rankine_oval": ("rankine_oval", {"speed_contours": True}),
    "corner_flow_90deg": ("corner_flow_90deg", {"equipotentials": True}),
    "flat_plate_edge": ("flat_plate_edge", {}),
}


# --- Closed-form solutions: velocity (u, v) at points (x, y), Anderson's sign conventions ---------

def polar(x, y, x0=0.0, y0=0.0):
    dx, dy = x - x0, y - y0
    return np.hypot(dx, dy), np.arctan2(dy, dx)


def from_polar(vr, vt, theta):
    return vr * np.cos(theta) - vt * np.sin(theta), vr * np.sin(theta) + vt * np.cos(theta)


def uniform(x, y, speed, alpha_deg):
    a = math.radians(alpha_deg)
    return speed * math.cos(a) + 0 * x, speed * math.sin(a) + 0 * y


def source(x, y, strength, x0, y0):  # V_r = Lambda / (2 pi r)
    r, t = polar(x, y, x0, y0)
    return from_polar(strength / (2 * np.pi * r), 0 * r, t)


def vortex(x, y, circulation, x0, y0):  # V_theta = -Gamma / (2 pi r), Gamma > 0 clockwise
    r, t = polar(x, y, x0, y0)
    return from_polar(0 * r, -circulation / (2 * np.pi * r), t)


def cylinder(x, y, speed, radius, circulation=0.0):  # Anderson eqs. 3.88 and 3.124
    r, t = polar(x, y)
    vr = (1 - radius**2 / r**2) * speed * np.cos(t)
    vt = -(1 + radius**2 / r**2) * speed * np.sin(t) - circulation / (2 * np.pi * r)
    return from_polar(vr, vt, t)


def corner(x, y, strength):  # phi = V r^2 cos 2 theta (90 degree corner)
    r, t = polar(x, y)
    return from_polar(2 * strength * r * np.cos(2 * t), -2 * strength * r * np.sin(2 * t), t)


def plate_edge(x, y, strength):  # phi = V r^(1/2) cos(theta / 2), theta in [0, 2 pi)
    r, t = polar(x, y)
    t = np.mod(t, 2 * np.pi)
    return from_polar(0.5 * strength * r**-0.5 * np.cos(t / 2), -0.5 * strength * r**-0.5 * np.sin(t / 2), t)


def add(*fields):
    return sum(f[0] for f in fields), sum(f[1] for f in fields)


TWO_PI = 2 * np.pi
# name -> (description, exact velocity)
VALIDATION = {
    "uniform_45deg": ("Uniform flow, U = 1, α = 45°", lambda x, y: uniform(x, y, 1, 45)),
    "source_sink_vortex": ("Source Λ = 2π, sink Λ = −2π, vortex Γ = 2π",
                           lambda x, y: add(source(x, y, TWO_PI, -2, 0), source(x, y, -TWO_PI, 0, 0),
                                            vortex(x, y, TWO_PI, 2, 0))),
    "cylinder": ("Cylinder: uniform + doublet μ = 1 (a = 1)", lambda x, y: cylinder(x, y, 1, 1)),
    "lifting_cylinder": ("Lifting cylinder, Γ = 2π", lambda x, y: cylinder(x, y, 1, 1, TWO_PI)),
    "rankine_half_body": ("Rankine half-body", lambda x, y: add(uniform(x, y, 1, 0), source(x, y, TWO_PI, -1, 0))),
    "rankine_oval": ("Rankine oval", lambda x, y: add(uniform(x, y, 1, 0), source(x, y, TWO_PI, -1, 0),
                                                      source(x, y, -TWO_PI, 1, 0))),
    "source_near_wall": ("Source Λ = 2π at (0, 1) above the wall y = 0 (exact: image source at (0, −1))",
                         lambda x, y: add(source(x, y, TWO_PI, 0, 1), source(x, y, TWO_PI, 0, -1))),
    "corner_flow_90deg": ("Sector flow, 90° corner (n = 2)", lambda x, y: corner(x, y, 1)),
    "flat_plate_edge": ("Sector flow, 360° (flow round a plate edge, n = 1/2)", lambda x, y: plate_edge(x, y, 1)),
}


def run_exe(exe, scene, **exports):
    args = [str(exe), str(scene)]
    for option, path in exports.items():
        args += [f"--export-{option}", str(path)]
    subprocess.run(args, check=True)


def scene_variant(name, overrides, work):
    scene = json.loads((EXAMPLES / f"{name}.json").read_text(encoding="utf-8"))
    scene["plot"].update({"particles": False, **overrides})
    path = work / f"{name}.json"
    path.write_text(json.dumps(scene, indent=2), encoding="utf-8")
    return path


def load_field(path):
    data = np.genfromtxt(path, delimiter=",", names=True)
    return {name: data[name] for name in data.dtype.names}


def validate(field, exact):
    """Largest velocity error relative to max(|V_exact|, median |V_exact|) over the defined nodes."""
    ok = np.isfinite(field["u"])
    x, y = field["x"][ok], field["y"][ok]
    ue, ve = exact(x, y)
    speed = np.hypot(ue, ve)
    scale = np.maximum(speed, np.median(speed))
    error = np.hypot(field["u"][ok] - ue, field["v"][ok] - ve) / scale
    return int(ok.sum()), int((~ok).sum()), float(error.max())


def cp_figure(fields, out):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    surface, ink, ink2, grid = "#fcfcfb", "#0b0b0b", "#52514e", "#e4e3df"
    series = [("cylinder", "Γ = 0", "#2a78d6", 0.0), ("lifting_cylinder", "Γ = 2π", "#eb6834", TWO_PI)]

    fig, ax = plt.subplots(figsize=(6.4, 3.6), dpi=160, facecolor=surface)
    ax.set_facecolor(surface)
    for name, label, colour, gamma in series:
        f = fields[name]
        column = np.isclose(f["x"], f["x"][np.argmin(np.abs(f["x"]))]) & (f["y"] >= 1.0)
        ys = np.linspace(1.0, f["y"][column].max(), 200)
        u_exact = 1 + 1 / ys**2 + gamma / (2 * np.pi * ys)  # V_theta on the +y axis, sign flipped
        cp_exact = 1 - u_exact**2
        ax.plot(ys, cp_exact, color=colour, linewidth=2, label=f"{label}, exact", zorder=2)
        ax.plot(f["y"][column], f["cp"][column], linestyle="none", marker="o", markersize=5,
                markerfacecolor=colour, markeredgecolor=surface, markeredgewidth=1.5,
                label=f"{label}, program", zorder=3)
        ax.annotate(label, (ys[-1], cp_exact[-1]), xytext=(6, 0), textcoords="offset points",
                    va="center", color=ink2, fontsize=9)

    ax.set_xlabel("y  (on the line x = 0, above the cylinder r = 1)", color=ink2, fontsize=9)
    ax.set_ylabel("Pressure coefficient Cp", color=ink2, fontsize=9)
    ax.set_title("Cp above the cylinder: program samples vs. exact solution", color=ink, fontsize=10, loc="left")
    ax.grid(True, color=grid, linewidth=0.8)
    ax.tick_params(colors=ink2, labelsize=8)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(grid)
    ax.set_xlim(1.0, None)
    ax.legend(frameon=False, fontsize=8, labelcolor=ink2, loc="lower right")
    fig.tight_layout()
    fig.savefig(out, facecolor=surface)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", type=Path, default=ROOT / "build/msvc/bin/Release/PotentialFlowLab.exe")
    parser.add_argument("--out", type=Path, default=ROOT / "docs/figures")
    parser.add_argument("--work", type=Path, default=ROOT / "build/report")
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")  # the table has Greek letters; Windows consoles may not
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=True)

    for figure, (name, overrides) in FIGURES.items():
        print(f"figure {figure}")
        run_exe(args.exe, scene_variant(name, overrides, args.work), image=args.out / f"{figure}.png")

    fields = {}
    rows = ["| Case | Defined nodes | Masked / singular nodes | Max. relative velocity error |",
            "|---|---:|---:|---:|"]
    for name, (description, exact) in VALIDATION.items():
        csv = args.work / f"{name}_field.csv"
        run_exe(args.exe, scene_variant(name, {}, args.work), field=csv)
        fields[name] = load_field(csv)
        defined, masked, error = validate(fields[name], exact)
        rows.append(f"| {description} | {defined} | {masked} | {error:.1e} |")
    table = "\n".join(rows)
    print(table)
    (args.out / "validation.md").write_text(table + "\n", encoding="utf-8")

    cp_figure(fields, args.out / "cylinder_cp.png")
    print(f"figures written to {args.out}")


if __name__ == "__main__":
    main()
