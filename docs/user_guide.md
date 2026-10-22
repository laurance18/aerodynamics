# Potential Flow Lab: User Guide

Potential Flow Lab builds two-dimensional, incompressible, irrotational flows by adding up
elementary solutions, and redraws the streamlines every time the configuration changes.

## Getting started

Run `PotentialFlowLab.exe`. It needs no installation; Windows 10/11 with an OpenGL 3.3 graphics
driver is enough. To start from an example, use **File → Open...** and pick a file from
`examples/`, or drag a scene file onto the exe.

The window has four parts:

| Part | What it does |
|---|---|
| Toolbar (left) | One icon per element type. Hover for its name, click to add one. |
| Plot (centre) | The flow in the (x, y) plane: streamlines, arrows, element markers. |
| Menu bar (top) | **File**, **Elements** and **View** menus. |
| Status bar (bottom) | Velocity under the mouse, streamline count, wall check, warnings. |

## The flow view

- **Zoom** with the mouse wheel and **pan** by dragging. Both axes always use the same scale, so
  circles look like circles. **View → Reset view** returns to −4 < x < 4, −2.5 < y < 2.5.
- Streamlines are drawn evenly spaced, with arrows in the flow direction. They are recomputed
  after every change and whenever the view moves.
- Hover the plot to read x, y, the velocity (u, v), the speed |V| and, when a uniform flow is
  present, the pressure coefficient Cp = 1 − |V|²/U∞² in the status bar.
- Hover an element marker to see its parameters; **double-click** it to edit it.
- Regions outside a sector flow's wedge are darkened: the flow is not defined there.

## Adding flow elements

Click a toolbar icon. A dialog opens with default values; change them and press **Apply** (or
**Cancel** / Esc). New elements are placed at the centre of the view. Angles are in degrees,
measured counter-clockwise from +x. If a value is invalid, Apply is disabled and the dialog says
why; if the result is legal but noteworthy (for example two singularities at the same point) a
yellow warning appears before you apply.

| Element | Parameters | Sign convention |
|---|---|---|
| Uniform flow | speed U∞ ≥ 0, direction α | u = U∞ cos α, v = U∞ sin α |
| Source / sink | strength m = Λ, position (x₀, y₀) | Λ > 0 source, Λ < 0 sink (Λ = flow rate per unit depth) |
| Point vortex | circulation Γ, position | Γ > 0 turns **clockwise** (Anderson) |
| Doublet | strength μ, position, axis angle β | F = μ e^(iβ)/(z − z₀); β = α with a uniform flow at α gives a cylinder of radius √(μ/U∞) |
| Sector (wedge) flow | strength V, vertex, wall angles θ₁, θ₂ | 0 < θ₂ − θ₁ ≤ 360°, n = 180°/(θ₂ − θ₁) |
| Symmetry wall | a point on the wall, angle φ | infinite straight wall with zero normal velocity |

To change or remove an element later, open the **Elements** menu: every element is listed with
its parameters; hover one and choose **Edit...** or **Delete**. **Delete all** (or **File → Clear
All**) removes every element but keeps the view.

## Streamlines and tracer particles

The **View** menu holds the plot settings, which are saved with the scene:

- **Streamline density**: number of streamlines across the height of the view.
- **Direction arrows** and **Arrow spacing** (screen pixels between arrows on a line).
- **Show wall images**: draw the mirrored elements that enforce the symmetry walls (faded).
- **Tracer particles** and **Particle trails**: animated particles coloured by speed (blue slow,
  red fast). They are a visual aid only; the streamlines are the result.

## Symmetry walls

A symmetry wall is an infinite straight line that the flow cannot cross. The program enforces it
with the method of images rather than just drawing a line:

- sources, vortices and doublets are mirrored across the wall (vortices with −Γ, doublets with
  axis angle 2φ − β);
- a uniform flow keeps only its component along the wall (with walls that cross, none of it);
- sector flows are not mirrored, so a wall is a streamline of a sector only if it lies along one
  of the sector's own walls (the status bar warns otherwise).

The status bar shows the numerical check: the largest |v · n| sampled along each wall in view,
divided by the largest speed there. It should be around 10⁻¹⁶. Two parallel walls need an
infinite row of images, so the program truncates it and warns that the wall condition is then
approximate.

## Saving and loading configurations

**File → Save** (Ctrl+S) and **Save As...** (Ctrl+Shift+S) write the scene as a JSON file: every
element with its parameters, the walls, the current view and the View-menu settings. **File →
Open...** (Ctrl+O) restores it exactly. **File → New** (Ctrl+N) starts an empty scene with
default settings. The title bar shows `*` when there are unsaved changes, and New, Open and Quit
ask before discarding them.

Files are plain text and can be edited by hand; angles are stored in degrees. A file with an
invalid element (for example θ₂ ≤ θ₁) is rejected with a message naming the element.

## Troubleshooting

- **"OpenGL 3.3 required" at start-up**: update the graphics driver, and run the program on the
  computer itself, not through Remote Desktop or a virtual machine.
- **Greek letters show as "?"**: the program uses the Segoe UI font from Windows; if it is
  missing, it falls back to a basic font without Greek letters.
- **(!) warnings in the status bar**: hover them for the full text. They describe legal but easily
  misread configurations (coincident singularities, elements on a wall, truncated images).
