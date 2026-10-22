#include "core/flow_field.hpp"

#include <algorithm>
#include <cmath>

namespace pfl {
namespace {

bool within(const FlowElement& element, Vec2 p, double radius) {
    const std::optional<Vec2> centre = singularPoint(element);
    return centre && lengthSquared(p - *centre) < radius * radius;
}

} // namespace

Complex FlowField::complexVelocity(Vec2 p) const {
    Complex w{};
    for (const FlowElement& element : elements) {
        if (!within(element, p, singularRadius)) w += pfl::complexVelocity(element, p);
    }
    return w;
}

Complex FlowField::complexPotential(Vec2 p) const {
    Complex f{};
    for (const FlowElement& element : elements) {
        if (!within(element, p, singularRadius)) f += pfl::complexPotential(element, p);
    }
    return f;
}

bool FlowField::inDomain(Vec2 p) const {
    return std::ranges::all_of(elements, [p](const FlowElement& e) { return inElementDomain(e, p); });
}

bool FlowField::nearSingularity(Vec2 p, double radius) const {
    return std::ranges::any_of(elements, [p, radius](const FlowElement& e) { return within(e, p, radius); });
}

double typicalSpeed(const FlowField& field, const Rect& region, int gridSize) {
    const Vec2 size = region.size();
    const double guard = 0.02 * std::max(size.x, size.y);
    std::vector<double> speeds;
    speeds.reserve(static_cast<std::size_t>(gridSize) * static_cast<std::size_t>(gridSize));
    for (int j = 0; j < gridSize; ++j) {
        for (int i = 0; i < gridSize; ++i) {
            const Vec2 p = region.min + Vec2{size.x * (i + 0.5) / gridSize, size.y * (j + 0.5) / gridSize};
            if (!field.inDomain(p) || field.nearSingularity(p, guard)) continue;
            const double s = length(field.velocity(p));
            if (std::isfinite(s)) speeds.push_back(s);
        }
    }
    if (speeds.empty()) return 0.0;
    auto middle = speeds.begin() + static_cast<std::ptrdiff_t>(speeds.size() / 2);
    std::ranges::nth_element(speeds, middle);
    return *middle;
}

} // namespace pfl
