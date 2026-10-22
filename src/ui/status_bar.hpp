// status_bar.hpp: bottom status bar.
//
// Shows the probe under the mouse (x, y, u, v, |V| and Cp when a free stream defines it), the
// streamline count, the numerical wall check (largest |v . n| relative to |V| along each symmetry
// wall), and the scene warnings (count, with the full list on hover).
#pragma once

#include "core/vec2.hpp"

#include <imgui.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace pfl {

struct ProbeInfo {
    Vec2 position;
    bool inDomain = true;
    Vec2 velocity;
    std::optional<double> pressureCoefficient;
};

struct WallStatus {
    std::size_t number = 0; // 1-based element number
    double relativeNormalSpeed = 0.0;
    int samples = 0;
};

struct StatusInfo {
    std::optional<ProbeInfo> probe;
    std::size_t streamlineCount = 0;
    std::vector<WallStatus> walls;
    std::vector<std::string> warnings;
};

float statusBarHeight();

void drawStatusBar(ImVec2 pos, ImVec2 size, const StatusInfo& info);

} // namespace pfl
