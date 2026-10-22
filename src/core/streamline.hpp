// streamline.hpp: evenly spaced streamlines with direction arrows.
//
// Streamlines are integrated with classic RK4 along the unit direction field v/|v| (arc-length
// steps, so slow and fast regions get the same resolution). Seeding follows Jobard & Lefer (1997):
// new seeds are placed one separation distance beside existing lines, and a line stops when it
// comes within half a separation of another one, so the picture has an even density.
//
// A line also stops when it leaves the bounds or the flow domain, comes within singularRadius of a
// singular point, reaches a stagnation point (speed below 1e-6 of the typical speed, or the
// direction reversing within one step), closes on itself, or reaches maxPointsPerLine.
//
// With `orthogonal` set, the same tracer follows the rotated field perp(v)/|v| instead. Those curves
// cross every streamline at right angles, and since grad phi = v is normal to them, phi is constant
// along each: they are the equipotential lines. Tracing them (rather than contouring phi) avoids the
// branch cut of phi around a vortex, where phi is multivalued.
#pragma once

#include "core/flow_field.hpp"
#include "core/vec2.hpp"

#include <vector>

namespace pfl {

struct StreamlineSettings {
    Rect bounds;                  // trace only inside this rectangle
    double separation = 0.25;     // distance between neighbouring streamlines
    double singularRadius = 0.02; // stop this close to a singular point
    double arrowSpacing = 1.0;    // arc length between direction arrows on a line; <= 0 for none
    int maxPointsPerLine = 5000;
    bool orthogonal = false;      // trace equipotential lines instead of streamlines
};

struct Arrow {
    Vec2 position;
    Vec2 direction; // unit vector along the flow
};

struct StreamlineSet {
    std::vector<std::vector<Vec2>> lines; // each ordered in the flow direction
    std::vector<Arrow> arrows;
};

StreamlineSet computeStreamlines(const FlowField& field, const StreamlineSettings& settings);

} // namespace pfl
