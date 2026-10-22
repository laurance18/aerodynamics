// data_export.hpp: plain-text CSV exports for post-processing (MATLAB, Excel, Python).
//
// Field data: one row per node of a regular grid (nodes on the rectangle's edges included) with
//     x, y, u, v, speed, phi, psi, cp
// Numbers are written in the shortest form that reads back as the same double. "NaN" marks nodes
// outside the flow domain or at a singular point, and cp where no free stream defines it. phi and psi use the principal branch of log, so phi jumps by Gamma across the line
// running in -x from each vortex, and psi by Lambda across the same line from each source.
//
// Lines: one row per point of every streamline and equipotential line,
//     kind, line, x, y
// with kind "streamline" or "equipotential" and line numbered from 1 within each kind.
//
// Surface pressure: one row per point of every body surface (see core/surface_pressure.hpp),
//     body, closed, x, y, s, streamwise, theta_deg, cp, cp_exact
// with body numbered from 1, closed 1 or 0, s the arc length from the stagnation point, streamwise
// x/c for a closed body (distance downstream of the stagnation point for a half-body), theta_deg the
// angle around a closed body from the free-stream direction, and cp_exact the closed-form Cp for a
// circular cylinder. Columns that do not apply are NaN.
#pragma once

#include "core/scene.hpp"
#include "core/streamline.hpp"
#include "core/surface_pressure.hpp"

#include <vector>

#include <filesystem>
#include <string>
#include <string_view>

namespace pfl {

std::string fieldCsv(const FlowModel& model, const Rect& bounds, int columns, int rows);
std::string linesCsv(const StreamlineSet& streamlines, const StreamlineSet& equipotentials);
std::string surfaceCsv(const std::vector<BodySurface>& bodies, const FlowModel& model);

// Writes `text` to `path`, replacing the file. On failure returns false and sets `error`.
bool writeTextFile(const std::filesystem::path& path, std::string_view text, std::string& error);

} // namespace pfl
