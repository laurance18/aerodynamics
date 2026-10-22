// plot_view.hpp: the main (x, y) plot.
//
// An ImPlot plot with labelled axes and equal x/y scales (scroll to zoom, drag to pan). Drawn
// back to front: the optional |V| bands, the region outside the flow domain (darkened), the
// tracer-particle layer (a texture the app renders), the optional equipotential lines, streamlines
// with direction arrows, sector and symmetry walls, one marker per element (wall images faded) and
// the |V| colour scale. Hovering a marker names the element; double-clicking it
// asks the caller to edit it.
#pragma once

#include "core/flow_field.hpp"
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

// Filled velocity-magnitude contours: |V| sampled on a grid over the view, drawn in equal bands
// from 0 to maxSpeed (faster cells, including those at singular points, use the top band).
struct SpeedMap {
    ScalarGrid speed;
    double maxSpeed = 0.0; // 0: nothing to show
};

// What the app computes from the flow for the plot to draw.
struct PlotLayers {
    const StreamlineSet& streamlines;
    const StreamlineSet& equipotentials; // empty when they are switched off
    const SpeedMap& speed;
    ParticleLayer particles;
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

    PlotViewResult draw(ImVec2 pos, ImVec2 size, const Scene& scene, const FlowModel& model, const PlotLayers& layers);

private:
    std::optional<Rect> m_pendingView;
};

} // namespace pfl
