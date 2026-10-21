// demo_panel.hpp: left-hand panel of the toolchain demo.
//
// Shows frame timing (text + ImPlot graph), the particle display settings, and a button that
// opens a modal dialog for the vortex strength. The dialog edits a pending copy of the flow
// parameters: Apply commits it, Cancel (or Esc) throws it away.
#pragma once

#include "core/dummy_flow.hpp"

#include <array>
#include <cstddef>

namespace pfl {

struct DemoSettings {
    DemoFlowParams flow;
    bool trails = true;
    float trailFade = 0.08f; // fraction of the trail removed per frame
    float pointSize = 2.0f;  // pixels
};

class DemoPanel {
public:
    // Returns the panel width in pixels so the caller can lay out the rest of the window.
    float draw(DemoSettings& settings, std::size_t particleCount);

private:
    void drawStrengthDialog(DemoSettings& settings);
    void recordFrameTime(float ms);

    DemoFlowParams m_pending;
    std::array<float, 240> m_frameMs{};
    int m_frameHead = 0;
};

} // namespace pfl
