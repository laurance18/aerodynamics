# Potential Flow Lab: An Interactive 2D Potential-Flow Simulator

**Semester Project 1, Mechanical Engineering, Fall 2026**
Deniz Işık

## 1. Objective and scope

The goal is a graphical program that builds two-dimensional, incompressible, irrotational flows by
superposing elementary solutions chosen by the user. Every time the configuration changes, the
program redraws the streamlines with arrows showing the flow direction. Potential Flow Lab provides
the six required building blocks: uniform flow, source/sink, point vortex, sector (wedge) flow,
doublet and straight symmetry wall. All of the specified interface is implemented: an icon toolbar
with parameter dialogs, a labelled (x, y) plot, an *Elements* menu for editing and deleting, and a
*File* menu with save, open, new and clear. The program also includes the optional extensions:
equipotential lines, velocity-magnitude contours, zoom and pan, selectable element markers, Cp
under the mouse, and image and data export.

The program is written in C++20 (the brief allows Python, MATLAB, Julia or an equivalent language)
and uses Dear ImGui/ImPlot for the interface and OpenGL for the tracer particles. It ships as a
single self-contained Windows executable. The user guide (`docs/user_guide.md`) covers operation;
this report covers the mathematics, the architecture and the verification.

## 2. Theory

### 2.1 Complex potential and superposition

For a 2D incompressible, irrotational flow the velocity potential φ and the stream function ψ both
satisfy Laplace's equation. They combine into the complex potential F(z) = φ + iψ, an analytic
function of z = x + iy, whose derivative is the complex velocity

$$W(z) = \frac{dF}{dz} = u - i v .$$

Laplace's equation is linear, so the velocity of a combination of elementary flows is the sum of
their velocities. The program therefore evaluates

$$W(z) = \sum_k W_k(z), \qquad F(z) = \sum_k F_k(z)$$

at every point it needs. It never combines pictures of streamlines; the streamlines are always
traced through the summed velocity field (Section 4).

### 2.2 Elementary flows

With ζ = z − z₀ measured from the element's position, the elements are (sign conventions follow
Anderson, *Fundamentals of Aerodynamics*, ch. 3):

| Element | Parameters | F(z) | W(z) |
|---|---|---|---|
| Uniform flow | speed V∞ ≥ 0, direction α | V∞ e^(−iα) z | V∞ e^(−iα) |
| Source / sink | strength Λ (> 0 source, < 0 sink), z₀ | (Λ/2π) ln ζ | Λ / (2π ζ) |
| Point vortex | circulation Γ (> 0 clockwise), z₀ | (iΓ/2π) ln ζ | iΓ / (2π ζ) |
| Doublet | strength μ = κ/2π, z₀, axis angle β | μ e^(iβ) / ζ | −μ e^(iβ) / ζ² |
| Sector flow | V, vertex z₀, walls θ₁ < θ₂ | V (ζ e^(−iθ₁))ⁿ | V n e^(−iθ₁) (ζ e^(−iθ₁))ⁿ⁻¹ |

For the uniform flow, u = V∞ cos α and v = V∞ sin α. A source gives the radial velocity
V_r = Λ/(2πr), and a vortex the tangential velocity V_θ = −Γ/(2πr), so Γ > 0 turns clockwise.
A uniform flow at angle α plus a doublet with β = α is the flow past a circular cylinder of radius
a = √(μ/V∞). Section 6 checks this case in detail.

The sector flow uses the opening angle β_s = θ₂ − θ₁ (0 < β_s ≤ 2π) and the exponent n = π/β_s.
Its walls θ = θ₁ and θ = θ₂ are streamlines (ψ = 0). For n > 1 (β_s < π) the vertex is a stagnation
point (e.g. a 90° corner, n = 2). For n < 1 (β_s > π) the velocity is infinite at the vertex (e.g.
flow round the edge of a plate, β_s = 360°, n = ½). The fractional power ζⁿ is evaluated with the
local angle θ_ℓ = (arg ζ − θ₁) mod 2π, written as |ζ|ⁿ e^(inθ_ℓ). This places the branch cut of ζⁿ
along the first wall, outside the open wedge, so F is continuous everywhere inside it.

### 2.3 Pressure coefficient

When the scene contains a free stream of speed V∞ (the sum of its uniform flows), Bernoulli's
equation along the steady, irrotational flow gives

$$C_p = 1 - \frac{|V|^2}{V_\infty^2},$$

which the status bar shows under the mouse and the field export writes for every grid node. With
no uniform flow there is no physically meaningful reference speed, so Cp is left undefined.

## 3. Boundary conditions, domains and singularities

### 3.1 Symmetry walls: method of images

A symmetry wall is an infinite straight line through a point w at angle φ. It must carry zero
normal velocity. Drawing a line is not enough; the program enforces the condition with the method
of images. Reflection across the wall is

$$\mathbf{p}' = \mathbf{w} + 2\left[(\mathbf{p}-\mathbf{w})\cdot\mathbf{t}\right]\mathbf{t} - (\mathbf{p}-\mathbf{w}),
\qquad \mathbf{t} = (\cos\varphi, \sin\varphi).$$

The image of an element has the complex potential F_img(z) = conj F(z′) (Schwarz reflection). On
the wall z′ = z, so F + F_img = 2 Re F is real there: ψ is constant along the wall, which is
therefore a streamline. For each element this gives:

- a source keeps its strength Λ at the mirrored position;
- a vortex changes sign (−Γ), because reflection reverses the sense of rotation;
- a doublet keeps μ and its axis becomes β′ = 2φ − β;
- a uniform flow is not imaged. Its image would cancel the normal component and double the
  tangential one. Instead the program keeps the free stream that an infinite wall admits: the
  component along the wall for a single wall or parallel walls, and none for walls that cross.

With several walls, every copy is reflected again across every wall until no new copy appears.
For one wall, or two walls meeting at π/k, this closes after a finite number of images and the
condition is exact. For other cases, such as two parallel walls (an infinite row of images), the
series is truncated at 32 images per element and the program warns that the wall condition is
approximate. The status bar always shows a live numerical check: the largest |v·n| sampled along
each wall in view, divided by the largest speed there.

### 3.2 Sector flow: admissible domain and interaction with other elements

The sector term is defined only inside its wedge 0 ≤ θ_ℓ ≤ θ₂ − θ₁. The program treats the
*admissible flow domain* as the intersection of the wedges of all sector flows in the scene. Outside
it the field is undefined. That region is darkened in the plot, no streamlines, equipotentials or
particles enter it, the probe reports "outside the flow domain", and the export writes NaN. The
interaction rules, stated explicitly as the brief requires, are:

1. Other elements are superposed without restriction inside the domain. They generally cross the
   sector's walls, so **the sector walls remain streamlines of the sector term only**. The program
   does not try to impose the wedge walls on other elements.
2. Symmetry walls do not mirror sector flows. A symmetry wall is a streamline of the sector term
   only if it runs along one of the sector's walls through its vertex; otherwise the program warns.
3. Several sector flows are allowed and restrict the domain to the intersection of their wedges
   (with a warning).

### 3.3 Singularities and invalid input

The source, vortex and doublet terms, and the sector term for n < 1, are infinite at their
centres. Each element's term is skipped within 10⁻⁹ of its singular point, so a sum evaluated
there stays finite. The streamline tracer stops 3 pixels from a singular point, and the speed map
puts singular cells in its top band. Coincident singularities are legal; a source and a vortex at
the same point form a spiral vortex. They are reported as a warning, as is an element lying on a
symmetry wall, where a vortex is cancelled by its own image. Invalid parameters are rejected in
the dialog before they can be applied, and the message names the problem. Examples are a negative
speed, non-finite numbers, and θ₂ − θ₁ outside (0°, 360°]. Scene files with invalid elements are
rejected on loading in the same way.

## 4. Numerical methods

**Streamlines.** Streamlines are integral curves of the unit direction field v/|v|, integrated
with classical fourth-order Runge–Kutta in steps of arc length. Using arc length gives slow and
fast regions the same resolution. The step is one sixth of the line separation, reduced to 15% of
the distance to the nearest singular point. Seeding follows Jobard and Lefer (1997). New seeds are
placed one separation beside existing lines, and a line stops when it comes within half a
separation of another, which gives evenly spaced lines whose density the user sets. A line also
stops at the domain boundary, near singular points, at stagnation points (speed below 10⁻⁶ of the
median speed, or the direction reversing within one step) and when it closes on itself. Arrows are
placed along each line at a fixed screen spacing, with a phase offset per line. All lines are
recomputed after every add, edit or delete, and whenever the view changes.

**Equipotential lines.** Equipotentials are traced by the same integrator applied to the rotated
field v⊥/|v| = (−v, u)/|v|. Because ∇φ = v is normal to these curves, φ is constant along each of
them, and they cross every streamline at right angles. Tracing was chosen over contouring φ on a
grid because φ is multivalued around a vortex. Its principal branch jumps by Γ across a cut, and
a contouring algorithm would draw spurious lines along that cut. The traced lines are unaffected
(Fig. 4). They use the same spacing as the streamlines, so where the speed is close to the typical
value the two families form an approximately square flow net.

**Velocity-magnitude contours.** |V| is sampled at the centres of 4-pixel cells over the view and
drawn in ten equal bands of the Viridis colour map, from 0 to twice the median speed in the view.
A colour scale is shown in the plot.

**Tracer particles.** Optional animated particles use midpoint RK2. They are a visual aid only;
the streamlines are the result.

## 5. Software architecture

The code is split into libraries with one-way dependencies. The physics core has no graphics
dependency and is unit-tested on its own.

| Module | Contents | Depends on |
|---|---|---|
| `pfl_core` | elements, superposition, images, scene model, streamline/equipotential tracer, speed grid, particles | C++ standard library only |
| `pfl_io` | JSON scene files (versioned, validated), CSV export | core, nlohmann/json |
| `pfl_render` | OpenGL particle layer | core, glad |
| `pfl_ui` | toolbar, menus, parameter dialog, plot view, status bar | core, Dear ImGui, ImPlot |
| `PotentialFlowLab` | window, main loop, wiring, file dialogs, PNG writer, command line | all of the above, GLFW |

**Data flow.** The interface edits a `Scene` (the element list plus the plot settings).
`buildFlowModel` turns it into the field that is actually evaluated: the original elements, their
wall images and the wall-constrained uniform flows. The streamline tracer, equipotential tracer,
speed map, particles, probe and exports all read this one `FlowField`, so whatever is drawn or
exported comes from a single source of truth.

**Interface.** The toolbar has one icon per element type, with the name on hover. A click opens a
modal dialog with Apply and Cancel, and Apply stays disabled until the input is valid. The plot
has labelled axes with equal scales, scroll-to-zoom and drag-to-pan. Hovering an element marker
names it, and double-clicking opens it for editing. The *Elements* menu lists every element with
its parameters, each with Edit and Delete. The *View* menu holds streamline density, arrows, wall
images, equipotentials, speed contours and particles. The *File* menu provides New, Open, Save,
Save As, Clear All and the three exports.

**Files and exports.** A scene is saved as a versioned JSON file containing every element with its
parameters, the walls, the view limits and all View-menu settings. Opening it restores all of
these. Exports are a PNG of the plot with axes, a CSV of x, y, u, v, |V|, φ, ψ and Cp on a
201-column grid, and a CSV of every streamline and equipotential polyline. Numbers are written in
the shortest form that reads back as the identical double. The same exports are available from
the command line (`PotentialFlowLab.exe scene.json --export-image out.png --export-field out.csv`),
which `tools/report_figures.py` uses to produce every figure and number in Section 6.

## 6. Verification

The unit-test suite (doctest, 24 test cases, 4541 assertions, all passing) covers each minimum
verification case of the brief. The script `tools/report_figures.py` adds an independent check. It
exports the program's field on a 201 × 117 grid and compares it with the classical closed-form
solutions, written in polar form in Python without reference to the program's complex-potential
code.

| Brief's verification case | Check | Result |
|---|---|---|
| Uniform flow, α = 0° and 45° | velocity at (3, −2); every streamline straight and parallel to (1, 1); every arrow along (√½, √½) | (0.707107, 0.707107); all lines and arrows pass |
| Source, sink, vortex signs | Λ = 2π at (1, 0); Γ = 2π at (1, 0) and (0, 1); vortex streamlines | (1, 0); (0, −1) and (1, 0), i.e. clockwise; closed circles, radius drift < 10⁻⁵ |
| Uniform flow + doublet (cylinder) | V∞ = 1, μ = 1: velocity at (0, a); V_r on r = a for α = 30°; Cp along x = 0 | (2, 0); V_r = 0 and V_θ = −2V∞ sin(θ − α); Fig. 1 |
| Cylinder with circulation | Γ = 2π: velocity at (0, a); stagnation points | (3, 0); stagnation at 210° and 330° (sin θ = −Γ/4πV∞a) |
| Symmetry wall | max \|v·n\| / \|V\| along the wall, 200 samples | 1.9 × 10⁻¹⁶ (inclined wall with every element type); 2.1 × 10⁻¹⁶ and 1.2 × 10⁻¹⁶ (two perpendicular walls) |
| Save, reopen | all six element types plus plot settings, saved and reloaded | identical element list and settings, identical velocities; all 9 example scenes load |

Further tests check that W = dF/dz numerically for every element type, and that the sector walls
carry zero normal velocity. For the equipotentials, φ varies by less than 10⁻⁴ of the line spacing
Δφ along each traced line, each step crosses the local velocity within 0.06° of a right angle,
and the lines around a vortex are exactly radial even though φ jumps by Γ across its branch cut.

Comparison of the exported field with the closed-form solutions (maximum of |V_program − V_exact|
relative to max(|V_exact|, median speed) over all defined grid nodes):

| Case | Defined nodes | Masked / singular nodes | Max. relative velocity error |
|---|---:|---:|---:|
| Uniform flow, U = 1, α = 45° | 23517 | 0 | 0 |
| Source Λ = 2π, sink Λ = −2π, vortex Γ = 2π | 23514 | 3 | 1.0 × 10⁻¹⁵ |
| Cylinder: uniform + doublet μ = 1 (a = 1) | 23516 | 1 | 1.0 × 10⁻¹⁵ |
| Lifting cylinder, Γ = 2π | 23516 | 1 | 1.2 × 10⁻¹⁵ |
| Rankine half-body | 23516 | 1 | 6.5 × 10⁻¹⁶ |
| Rankine oval | 23515 | 2 | 8.3 × 10⁻¹⁶ |
| Source above the wall y = 0 (exact: image source) | 23517 | 0 | 8.2 × 10⁻¹⁶ |
| Sector flow, 90° corner (n = 2) | 16308 | 7008 | 3.9 × 10⁻¹⁶ |
| Sector flow, 360° plate edge (n = ½) | 23516 | 1 | 4.2 × 10⁻¹⁶ |

The program reproduces every closed-form solution to machine precision. The masked nodes are the
grid nodes that fall on a singular point, plus, for the 90° corner, every node outside the wedge
x ≥ 0, y ≥ 0.

<p align="center"><img src="figures/cylinder_cp.png" width="70%" alt="Cp along the line x = 0 above the cylinder: program samples on the exact curves for Γ = 0 and Γ = 2π"></p>

*Figure 1. Pressure coefficient along x = 0 above the cylinder (a = 1, V∞ = 1). Markers: program
export. Lines: Cp = 1 − (1 + a²/y² + Γ/2πV∞y)². At the surface Cp = −3 without circulation and
−8 with Γ = 2π.*

## 7. Examples

<p align="center">
<img src="figures/cylinder.png" width="49%" alt="Cylinder in uniform flow with speed contours">
<img src="figures/lifting_cylinder.png" width="49%" alt="Lifting cylinder with equipotential lines">
</p>

*Figure 2 (left). Non-lifting cylinder with |V| contours: stagnation (dark) at θ = 0° and 180°,
|V| = 2V∞ at the top and bottom. Figure 3 (right). Lifting cylinder, Γ = 2π, with equipotential
lines (orange): the stagnation points move down to 210° and 330°.*

<p align="center">
<img src="figures/source_sink_vortex.png" width="49%" alt="Source, sink and vortex with equipotentials">
<img src="figures/source_near_wall.png" width="49%" alt="Source above a symmetry wall">
</p>

*Figure 4 (left). Source (green), sink (red) and clockwise vortex (orange) with equipotentials. The
equipotentials are radial around the vortex and continuous across its branch cut. Figure 5
(right). Source above the symmetry wall y = 0: no streamline crosses the wall. The image source
below the wall is outside the view.*

<p align="center">
<img src="figures/corner_flow_90deg.png" width="49%" alt="Flow into a 90 degree corner">
<img src="figures/rankine_oval.png" width="49%" alt="Rankine oval with speed contours">
</p>

*Figure 6 (left). Sector flow in a 90° corner (n = 2) with equipotentials. The region outside the
wedge is masked. Figure 7 (right). Rankine oval (uniform flow, source and sink) with |V|
contours.*

## 8. Limitations and conclusions

- RK4 with a fixed arc-length step is least accurate where the direction field turns quickly,
  near stagnation points and inside the doublet. Lines there may stop short or kink slightly (for
  example the dividing streamline on the cylinder surface is not drawn through the stagnation
  points). This affects the picture only; the field values are exact.
- Walls that do not close the image system (parallel walls, or angles other than π/k) are
  approximated with 32 images per element, and the program warns when this happens.
- The sector walls are streamlines of the sector term only, not of the superposed flow
  (Section 3.2). This is the defined behaviour, not a numerical error.
- The exported φ (with vortices) and ψ (with sources) use the principal branch of log and jump
  across a cut running in −x from each such element.

Potential Flow Lab meets the functional and physical requirements of the brief. The required
elements are superposed at the velocity level, symmetry walls are enforced with exact image systems
and verified to |v·n|/|V| ≈ 10⁻¹⁶, the sector flow's domain and interactions are defined and
enforced, and the interface provides the required toolbar, dialogs, Elements and File menus, and
saved configurations. Every elementary and combined flow tested matches its closed-form solution to
machine precision.

## References

1. J. D. Anderson, *Fundamentals of Aerodynamics*, 6th ed., McGraw-Hill, 2017, ch. 3.
2. B. Jobard and W. Lefer, "Creating evenly-spaced streamlines of arbitrary density," in
   *Visualization in Scientific Computing '97*, Springer, 1997, pp. 43–55.
