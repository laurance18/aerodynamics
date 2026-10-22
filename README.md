# Potential Flow Lab

An interactive 2D potential-flow simulator in C++20, built as a senior mechanical-engineering
semester project. It superposes elementary flows (uniform flow, source/sink, vortex, doublet,
sector flow) and straight symmetry walls, draws evenly spaced streamlines with direction arrows,
equipotential lines and velocity-magnitude contours, and animates tracer particles on the GPU.

- Left toolbar: one icon per element type (hover for the name); a click opens a parameter dialog
  with Apply / Cancel.
- Main plot: labelled (x, y) axes, scroll to zoom, drag to pan; streamlines are recomputed after
  every add, edit or delete. Hover an element to name it, double-click it to edit it.
- **Elements** menu: every element with its parameters, each with Edit and Delete.
- **File** menu: New, Open, Save, Save As (JSON scenes, see `examples/`), Clear All, and exports:
  PNG image of the plot, CSV field data (u, v, |V|, φ, ψ, Cp on a grid), CSV streamline and
  equipotential polylines. The exports also run from the command line.
- **View** menu: streamline density, arrows, wall images, equipotential lines, speed contours,
  tracer particles.
- Status bar: velocity and Cp under the mouse, the numerical wall check, and warnings.

See [docs/user_guide.md](docs/user_guide.md) for usage, [docs/report.md](docs/report.md) for the
project report (equations, architecture, validation, examples) and
[docs/report_notes.md](docs/report_notes.md) for the working notes behind it.

## Report figures and validation

```powershell
python tools/report_figures.py     # needs numpy and matplotlib, and a Release build
```

This renders the report figures into `docs/figures/` with the exe's command-line exports and
compares the exported fields with the closed-form solutions (`docs/figures/validation.md`).

The release build is one self-contained `PotentialFlowLab.exe`. It needs no installer, DLLs,
Visual C++ Redistributable or extra folders.

## Prerequisites (Windows 10/11 x64)

| Tool | Notes |
|---|---|
| Visual Studio 2022 with the **Desktop development with C++** workload | Provides MSVC and the Windows SDK |
| CMake ≥ 3.25 | Put it on `PATH` (cmake.org installer or `winget install Kitware.CMake`) |
| Git | Used by CMake to fetch dependencies |
| VS Code + **CMake Tools** + **C/C++** extensions | Optional; recommended in `.vscode/extensions.json` |

All libraries are downloaded and built from source on the first configure, so the first configure
needs internet. Nothing else has to be installed.

## Build, run, test (PowerShell, from the repository root)

```powershell
cmake --preset msvc                     # configure (Visual Studio 2022 generator, x64)
cmake --build --preset debug            # or: cmake --build --preset release
.\build\msvc\bin\Debug\PotentialFlowLab.exe
ctest --preset debug                    # unit tests (doctest), also: ctest --preset release
```

A plain PowerShell works; you don't need a Developer Prompt. Outputs land in
`build\msvc\bin\<Config>\`.

### VS Code

1. Open the repository folder. CMake Tools detects `CMakePresets.json` and configures it.
2. In the status bar, pick configure preset **msvc** and build preset **debug**.
3. Press **F5** ("Debug PotentialFlowLab"). It builds first, then starts the debugger.
4. Tests: the CMake Tools *Testing* panel, or the "Debug unit tests" launch configuration.

### Shader hot reload (Debug only)

Debug builds read the GLSL files straight from `shaders/`. Edit a shader, then press **Ctrl+R**
(or *File → Reload shaders*). A compile error is printed to the console, and the previous working
shader stays active. Release builds use only the copies embedded in the exe. To make Debug use
the embedded copies too, configure with `-DPFL_SHADERS_FROM_DISK=OFF`.

## Packaging for distribution

```powershell
cmake --build --preset release --target dist
```

This produces `dist\PotentialFlowLab.zip`, containing `PotentialFlowLab.exe` and `README.txt`.

### Verifying the exe is standalone

Open **Developer PowerShell for VS 2022**, which puts `dumpbin` on `PATH`:

```powershell
dumpbin /dependents build\msvc\bin\Release\PotentialFlowLab.exe
dumpbin /headers    build\msvc\bin\Release\PotentialFlowLab.exe | findstr subsystem
```

- The dependency list should contain **only Windows system DLLs**. Currently those are:
  `COMDLG32.dll ole32.dll KERNEL32.dll USER32.dll GDI32.dll SHELL32.dll IMM32.dll` (COMDLG32
  provides the Open / Save dialogs; ole32 starts the Windows Imaging Component that writes PNGs).
- `OPENGL32.dll` doesn't appear because GLFW loads it at runtime.
- Any `VCRUNTIME140*.dll`, `MSVCP140*.dll`, `api-ms-win-crt-*.dll` or `glfw3.dll` means the
  static linking broke.
- The subsystem should read `2 subsystem (Windows GUI)`, which means no console window.

As a final check, copy the exe alone into an empty folder on another PC and double-click it.

## How the standalone build works

- **Static runtime:** `CMAKE_MSVC_RUNTIME_LIBRARY = MultiThreaded$<$<CONFIG:Debug>:Debug>` (`/MT`,
  `/MTd`). It is set before any dependency is fetched, so they all use it.
- **Static dependencies:** `BUILD_SHARED_LIBS=OFF`. ImGui/ImPlot are compiled into `pfl_imgui`.
- **Embedded assets:** `cmake/EmbedResources.cmake` turns `shaders/*` into byte arrays inside the
  exe at build time, using a generated `.cpp` that is rebuilt when a shader changes. Byte arrays
  (not string literals) avoid MSVC's string-literal size limit and will also work for fonts and
  icons.
- **Windows resources:** `resources/app.rc.in` adds the app icon (`resources/app.ico`, regenerate
  with `python tools/make_icon.py`) and version info taken from `project(VERSION ...)`.
- **No console in Release:** the exe links with `/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup`, so
  `main()` stays a normal `main()`.
- **No OpenGL 3.3:** a Windows message box explains the requirement (and that VMs / Remote
  Desktop often lack it), then the program exits. To test this message in a Debug build, set
  `$env:PFL_FORCE_GL_FAIL=1` before running.
- **Laptops:** the exe exports `NvOptimusEnablement` / `AmdPowerXpressRequestHighPerformance` so
  hybrid-graphics laptops use the discrete GPU.

## Repository layout

```
src/core/     pure math/physics (no OpenGL/ImGui): flow elements, superposition, wall images,
              scene model and warnings, RK4 streamlines, tracer particles
src/render/   OpenGL wrappers: shaders (with error logs), buffers, framebuffer, particle layer
src/ui/       ImGui/ImPlot: menus, toolbar, parameter dialog, plot view, status bar
src/io/       JSON scene save/load (nlohmann/json)
src/app/      main(), window/context setup, main loop, Windows helpers
shaders/      GLSL sources (embedded at build time)
tests/        doctest unit tests (link only pfl_core and pfl_io)
resources/    icon, Windows .rc template, README.txt for the zip
cmake/        dependency pins, warnings, resource embedding, packaging
third_party/  generated glad loader (see third_party/glad/README.md)
tools/        helper scripts
docs/         user guide and report notes
examples/     example scenes (cylinder, lifting cylinder, Rankine bodies, corner flow, ...)
```

## Dependencies (pinned in `cmake/Dependencies.cmake`)

| Library | Version |
|---|---|
| GLFW | 3.5.1 |
| Dear ImGui (docking branch) | v1.93.0-docking |
| ImPlot | master @ `851cdbd` (2026-10-10). v1.0 plus fixes needed for ImGui ≥ 1.92.9 |
| nlohmann/json | v3.12.0 |
| doctest | v2.5.3 |
| glad | glad2 2.0.8, GL 4.6 core + KHR_debug (generated, committed) |

## OpenGL notes

The app requests an **OpenGL 3.3 core** context. On Windows, current NVIDIA, AMD and Intel
drivers all provide 4.6, so **4.3 compute shaders** are available for later GPU particle work.
Keep a 3.3 fallback anyway, because some machines (VMs, Remote Desktop, old drivers) are limited
to older versions. macOS tops out at 4.1, but this project targets Windows only.
