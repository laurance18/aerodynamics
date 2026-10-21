// dummy_flow.hpp: made-up analytic velocity field for the toolchain demo.
//
// A regularised point vortex at the origin plus a uniform drift along +x:
//     u(p) = S * perp(p) / (|p|^2 + eps) + (U, 0)
// Positive S turns counter-clockwise. Replaced by flow_field.hpp once real elements exist.
#pragma once

#include "core/vec2.hpp"

namespace pfl {

struct DemoFlowParams {
    double strength = 1.0; // S: vortex strength
    double drift = 0.3;    // U: uniform drift speed along +x
    double coreEps = 0.05; // eps: core regularisation, keeps the speed finite at the origin
};

Vec2 demoVelocity(Vec2 p, const DemoFlowParams& params);

// Upper bound of |u| over the plane, used to normalise the speed colormap.
double demoMaxSpeed(const DemoFlowParams& params);

} // namespace pfl
