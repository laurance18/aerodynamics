#include "core/dye.hpp"

namespace pfl {

std::vector<Vec2> emitterPositions(const DyeRake& rake) {
    std::vector<Vec2> positions;
    const int n = std::clamp(rake.emitters, 1, k_maxRakeEmitters);
    if (n == 1) return {(rake.start + rake.end) * 0.5};
    positions.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) positions.push_back(rake.start + (rake.end - rake.start) * (static_cast<double>(i) / (n - 1)));
    return positions;
}

std::optional<std::string> dyeRakeError(const DyeRake& rake) {
    if (!std::isfinite(rake.start.x) || !std::isfinite(rake.start.y) || !std::isfinite(rake.end.x) ||
        !std::isfinite(rake.end.y)) {
        return "The end points must be finite numbers.";
    }
    if (rake.emitters < 1 || rake.emitters > k_maxRakeEmitters) {
        return "The number of emitters must be between 1 and " + std::to_string(k_maxRakeEmitters) + ".";
    }
    return std::nullopt;
}

void DyeSystem::detach() {
    for (std::size_t i = 0; i < m_attached; ++i) m_streaks[i].attached = false;
    m_attached = 0;
}

void DyeSystem::clear() {
    m_streaks.clear();
    m_attached = 0;
    m_sinceRelease = 0.0;
}

} // namespace pfl
