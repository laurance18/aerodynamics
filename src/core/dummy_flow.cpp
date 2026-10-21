#include "core/dummy_flow.hpp"

#include <cmath>

namespace pfl {

Vec2 demoVelocity(Vec2 p, const DemoFlowParams& params) {
    const double denom = lengthSquared(p) + params.coreEps;
    return perp(p) * (params.strength / denom) + Vec2{params.drift, 0.0};
}

double demoMaxSpeed(const DemoFlowParams& params) {
    // |S| r / (r^2 + eps) peaks at r = sqrt(eps) with value |S| / (2 sqrt(eps)).
    return std::abs(params.strength) / (2.0 * std::sqrt(params.coreEps)) + std::abs(params.drift);
}

} // namespace pfl
