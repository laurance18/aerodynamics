// plot_view.hpp: the main (x, y) plot.
//
// An ImPlot plot with labelled axes and equal x/y scales (scroll to zoom, drag to pan). Drawn
// back to front: the optional |V| or Cp bands, the region outside the flow domain (darkened), the
// tracer-particle layer (a texture the app renders), the optional equipotential lines, streamlines
// with direction arrows, the body surfaces coloured by Cp with pressure arrows, sector and symmetry
// walls, one marker per element (wall images faded) and the colour scale. Hovering a marker names
// the element; double-clicking it asks the caller to edit it.
#pragma once

#include "core/flow_field.hpp"
#include "core/scene.hpp"
#include "core/streamline.hpp"
#include "core/surface_pressure.hpp"
#include "core/vec2.hpp"

#include <imgui.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace pfl {

struct ParticleLayer {
    ImTextureID texture = 0;             // 0: nothing to show
    ImDrawCallback beginBlend = nullptr; // sets premultiplied-alpha blending for the texture
};

// Filled contours of |V| or Cp: the quantity sampled on a grid over the view, drawn in `bands`
// equal bands from low to high. Values beyond either end use the end band (|V| is +infinity and Cp
// -infinity at singular points). |V| bands use Viridis, Cp bands the diverging pressureColor().
struct ContourMap {
    Background quantity = Background::None;
    ScalarGrid values;
    double low = 0.0;
    double high = 0.0;
    int bands = 0; // 0 with quantity Cp: no free stream, so Cp is undefined
};

// Body surfaces drawn coloured by Cp (on the scale lowestCp..1) with pressure arrows.
struct SurfaceLayer {
    const std::vector<BodySurface>& bodies; // empty when switched off
    double lowestCp = -3.0;
    std::optional<Vec2> marker; // the surface point hovered in the Cp panel
};

// What the app computes from the flow for the plot to draw.
struct PlotLayers {
    const StreamlineSet& streamlines;
    const StreamlineSet& equipotentials; // empty when they are switched off
    const ContourMap& contours;
    SurfaceLayer surfaces;
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
