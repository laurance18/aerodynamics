#include "core/flow_field.hpp"

#include <algorithm>

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

} // namespace pfl
