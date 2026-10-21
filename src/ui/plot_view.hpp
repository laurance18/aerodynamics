// plot_view.hpp: the main (x, y) plot.
//
// An ImPlot plot with labelled axes and equal x/y scales (scroll to zoom, drag to pan). Drawn
// back to front: the region outside the flow domain (darkened), the tracer-particle layer (a
// texture the app renders), streamlines with direction arrows, sector and symmetry walls, and one
// marker per element (wall images faded). Hovering a marker names the element; double-clicking it
// asks the caller to edit it.
#pragma once

#include "core/scene.hpp"
#include "core/streamline.hpp"
#include "core/vec2.hpp"

#include <imgui.h>

#include <cstddef>
#include <optional>

namespace pfl {

struct ParticleLayer {
    ImTextureID texture = 0;             // 0: nothing to show
    ImDrawCallback beginBlend = nullptr; // sets premultiplied-alpha blending for the texture
};

struct PlotViewResult {
    Rect limits;                   // world rectangle currently shown
    ImVec2 pixelSize{0.0f, 0.0f};  // size of the plot area in pixels
    std::optional<Vec2> mouse;     // world position under the mouse, if it is over the plot
    std::optional<std::size_t> editRequest;
};

class PlotView {
public:
    // Shows exactly `view` from the next frame on (after loading a file or resetting the view).
    void setView(const Rect& view) { m_pendingView = view; }

    PlotViewResult draw(ImVec2 pos, ImVec2 size, const Scene& scene, const FlowModel& model,
                        const StreamlineSet& streamlines, const ParticleLayer& particles);

private:
    std::optional<Rect> m_pendingView;
};

} // namespace pfl
