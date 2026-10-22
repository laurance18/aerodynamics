# Report Notes

Running notes for the semester report: equations, decisions and where they live in the code.

## Theory: elementary potential flows and superposition

Complex potential F(z) = φ + iψ, complex velocity W = dF/dz = u − iv. Laplace's equation is
linear, so the program sums W (and F) of all elements; it never combines streamline pictures.
With ζ = z − z₀ (`src/core/flow_element.cpp`):

| Element | W(z) | Notes |
|---|---|---|
| Uniform flow | V∞ e^(−iα) | α CCW from +x |
| Source / sink | Λ / (2π ζ) | Λ > 0 source |
| Point vortex | iΓ / (2π ζ) | Γ > 0 clockwise (Anderson §2.13) |
| Doublet | −μ e^(iβ) / ζ² | μ = κ/2π; β rotates the axis CCW |
| Sector, walls θ₁, θ₂ | V n e^(−iθ₁) \|ζ\|^(n−1) e^(i(n−1)θℓ) | n = π/(θ₂ − θ₁), θℓ = (arg ζ − θ₁) mod 2π |

Verification: all values of the reference sheet (section 5) are unit tests in
`tests/test_flow.cpp`, together with a numerical check that W = dF/dx = −i dF/dy for every element.

## Singularities

Source, vortex and doublet terms are skipped within 10⁻⁹ of their centre (and a sector vertex for
n < 1), so the sum stays finite there. Streamlines stop 3 px from a singular point, and particles
respawn when they get close. Coincident singularities are allowed (e.g. source + vortex = spiral
vortex) but reported as a warning.

## Sector flow: admissible domain and interaction with other elements

- The sector term is defined only inside its wedge 0 ≤ θℓ ≤ θ₂ − θ₁ (0 < θ₂ − θ₁ ≤ 2π). Computing θℓ
  explicitly keeps the branch cut of ζ^(n−1) outside the wedge.
- The flow domain is the intersection of all sector wedges. Outside it the field is masked:
  darkened in the plot, no streamlines, no particles, "outside the flow domain" in the probe.
- Other elements are superposed without restriction and therefore generally cross the sector walls;
  the sector walls are streamlines of the sector term only.
- Symmetry walls do not mirror sector flows. A symmetry wall is a streamline of the sector term only
  if it lies along one of the sector walls; otherwise the program warns.

## Method of images for walls

`src/core/image_system.cpp`. Reflection across the line through w at angle φ:
p' = w + 2((p − w)·t)t − (p − w), t = (cos φ, sin φ). Each image is F_img(z) = conj(F(reflect(z))):

- source: same Λ; vortex: −Γ; doublet: same μ, β' = 2φ − β.
- several walls: images are reflected again across every wall until no new copies appear. This
  closes for one wall and for walls meeting at π/k; otherwise (e.g. parallel walls) the series is
  truncated at 32 images per element and a warning says the wall condition is approximate.
- uniform flow: its image would double the component along the wall, so instead it is projected
  onto what the walls allow (the tangential component for parallel walls, zero for crossing walls).

Verification: `tests/test_scene.cpp` checks |v·n| / |V| < 10⁻¹² along a single inclined wall with
every point element present, and along two perpendicular walls. The status bar shows the same
check live for the current view.

## Numerical methods (RK4 streamlines, particle advection)

- Streamlines (`src/core/streamline.cpp`): classic RK4 on the unit direction field v/|v| with step
  = separation/6, reduced to 15% of the distance to the nearest singular point. Seeding follows
  Jobard & Lefer (1997): seeds one separation beside existing lines, lines stop half a separation
  from another line. Lines also stop at the domain boundary, singularities, stagnation points
  (|v| < 10⁻⁶ × median speed or direction reversal in one step) and when they close on themselves.
- Arrows: placed along each line every "arrow spacing" pixels, with a per-line phase offset.
- Equipotentials: the same tracer on the rotated field perp(v)/|v| (`StreamlineSettings::orthogonal`).
  grad φ = v is normal to these curves, so φ is constant along them. Chosen over contouring φ
  because φ is multivalued around a vortex (principal branch jumps by Γ across a cut).
  Same separation as the streamlines → approximate flow net.
- Speed contours: |V| sampled at 4 px cell centres (`sampleSpeed`), 10 Viridis bands from 0 to
  2 × median speed; NaN outside the domain, +∞ (top band) at singular points.
- Particles (`src/core/particles.hpp`): midpoint RK2 with sub-steps limited by distance travelled.
  Time is scaled so a particle at the median speed crosses the view in about 6 s.

## Software architecture

| Module | Contents | Depends on |
|---|---|---|
| `pfl_core` | elements, superposition, images, scene model, streamlines, particles | standard library only |
| `pfl_io` | JSON scene files (versioned, validated) | core, nlohmann/json |
| `pfl_render` | OpenGL particle layer (point sprites into an off-screen trail buffer) | core, glad |
| `pfl_ui` | ImGui/ImPlot menus, toolbar, dialog, plot view, status bar | core, ImGui/ImPlot |
| `PotentialFlowLab` | window, main loop, wiring, file dialogs | all of the above, GLFW |

Data flow: the UI edits a `Scene` (element list + plot settings) → `buildFlowModel` adds wall
images and constrains uniform flows → `FlowField` is evaluated by the streamline tracer, the
particles and the probe. Any edit rebuilds the model and recomputes the streamlines.

## Validation against analytic results

- Uniform flow at α = 0° and 45°: `uniform_45deg.json`, unit tests.
- Source, sink, vortex sign conventions: `source_sink_vortex.json`, unit tests.
- Cylinder (uniform + doublet), with circulation: `cylinder.json`, `lifting_cylinder.json`; unit
  tests for (2, 0) and (3, 0) at (0, 1), stagnation at 210°/330°, V_r = 0 on r = a for a rotated
  cylinder.
- Symmetry wall: `source_near_wall.json`, status-bar wall check, unit tests.
- Save / reopen: round-trip unit test, and every file in `examples/` is loaded by a test.
- Independent check: `tools/report_figures.py` compares the exported field CSVs with closed-form
  solutions written in polar form in numpy; all cases agree to ~1e-15 (`docs/figures/validation.md`).

## Results and screenshots

Figures are generated by `tools/report_figures.py` into `docs/figures/`; the write-up is
`docs/report.md`.
