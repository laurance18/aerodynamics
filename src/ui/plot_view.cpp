#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/plot_view.hpp"

#include "ui/style.hpp"

#include <implot.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace pfl {
namespace {

constexpr ImU32 k_streamlineColor = IM_COL32(170, 200, 240, 220);
constexpr ImU32 k_arrowColor = IM_COL32(210, 230, 255, 245);
constexpr ImU32 k_equipotentialColor = IM_COL32(245, 175, 95, 175);
constexpr ImU32 k_particleTint = IM_COL32(115, 115, 115, 115);
constexpr ImU32 k_maskColor = IM_COL32(0, 0, 0, 120);
constexpr ImU32 k_hintColor = IM_COL32(200, 200, 210, 160);
constexpr ImU32 k_highlightColor = IM_COL32(255, 255, 255, 230);
constexpr float k_imageAlpha = 0.35f;
constexpr float k_maskCellPx = 6.0f;
constexpr float k_hitRadiusPx = 10.0f;
constexpr int k_speedBands = 10;
constexpr float k_speedAlpha = 0.6f;

ImVec2 toPixels(Vec2 p) { return ImPlot::PlotToPixels(p.x, p.y); }

Vec2 toWorld(ImVec2 pixel) {
    const ImPlotPoint p = ImPlot::PixelsToPlot(pixel);
    return {p.x, p.y};
}

// Screen direction of a world angle (screen y points down).
ImVec2 screenDirection(double angle) { return {static_cast<float>(std::cos(angle)), static_cast<float>(-std::sin(angle))}; }

ImVec2 normalized(ImVec2 v) {
    const float l = std::sqrt(v.x * v.x + v.y * v.y);
    return l > 0.0f ? v / l : ImVec2(1.0f, 0.0f);
}

float distanceToSegment(ImVec2 p, ImVec2 a, ImVec2 b) {
    const ImVec2 ab = b - a;
    const float l2 = ab.x * ab.x + ab.y * ab.y;
    const float t = l2 > 0.0f ? std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / l2, 0.0f, 1.0f) : 0.0f;
    const ImVec2 d = p - (a + ab * t);
    return std::sqrt(d.x * d.x + d.y * d.y);
}

// Darkens every pixel cell whose centre is outside the flow domain (only sector flows restrict it).
// Cells are merged into horizontal runs to keep the draw list small.
void drawMask(ImDrawList* dl, const FlowField& field, ImVec2 pos, ImVec2 size) {
    const float cell = k_maskCellPx * uiScale();
    const int columns = static_cast<int>(std::ceil(size.x / cell));
    const int rows = static_cast<int>(std::ceil(size.y / cell));
    for (int j = 0; j < rows; ++j) {
        const float y0 = pos.y + j * cell;
        int runStart = -1;
        for (int i = 0; i <= columns; ++i) {
            const bool outside =
                i < columns && !field.inDomain(toWorld({pos.x + (i + 0.5f) * cell, y0 + 0.5f * cell}));
            if (outside && runStart < 0) runStart = i;
            if (!outside && runStart >= 0) {
                dl->AddRectFilled({pos.x + runStart * cell, y0}, {pos.x + i * cell, y0 + cell}, k_maskColor);
                runStart = -1;
            }
        }
    }
}

// Colour of speed band `band` (0 slowest) on the Viridis scale.
ImU32 speedBandColor(int band) {
    ImVec4 c = ImPlot::SampleColormap((band + 0.5f) / k_speedBands, ImPlotColormap_Viridis);
    c.w = k_speedAlpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}

// Filled |V| bands. Cells in the same band are merged into horizontal runs.
void drawSpeedBands(ImDrawList* dl, const SpeedMap& map) {
    const ScalarGrid& grid = map.speed;
    if (map.maxSpeed <= 0.0 || grid.columns <= 0 || grid.rows <= 0) return;
    const Vec2 cell{grid.bounds.size().x / grid.columns, grid.bounds.size().y / grid.rows};
    std::vector<float> xs(static_cast<std::size_t>(grid.columns) + 1);
    for (int i = 0; i <= grid.columns; ++i) xs[static_cast<std::size_t>(i)] = toPixels(grid.bounds.min + Vec2{cell.x * i, 0.0}).x;

    const auto bandOf = [&](int i, int j) {
        const double speed = grid.at(i, j);
        if (std::isnan(speed)) return -1; // outside the domain: the mask covers it
        // Clamp before converting: the speed is +infinity at singular points.
        return static_cast<int>(std::clamp(speed / map.maxSpeed * k_speedBands, 0.0, k_speedBands - 0.5));
    };
    for (int j = 0; j < grid.rows; ++j) {
        const float yBottom = toPixels(grid.bounds.min + Vec2{0.0, cell.y * j}).y;
        const float yTop = toPixels(grid.bounds.min + Vec2{0.0, cell.y * (j + 1)}).y;
        int runStart = 0;
        int runBand = bandOf(0, j);
        for (int i = 1; i <= grid.columns; ++i) {
            const int band = i < grid.columns ? bandOf(i, j) : -2;
            if (band == runBand) continue;
            if (runBand >= 0) {
                dl->AddRectFilled({xs[static_cast<std::size_t>(runStart)], yTop}, {xs[static_cast<std::size_t>(i)], yBottom},
                                  speedBandColor(runBand));
            }
            runStart = i;
            runBand = band;
        }
    }
}

// Colour scale for the |V| bands in the top-right corner of the plot.
void drawSpeedScale(ImDrawList* dl, const SpeedMap& map, ImVec2 plotPos, ImVec2 plotSize) {
    if (map.maxSpeed <= 0.0) return;
    const float s = uiScale();
    const float barWidth = 14.0f * s;
    const float bandHeight = 16.0f * s;
    const float font = ImGui::GetFontSize();
    const std::string top = std::format("≥ {:.3g}", map.maxSpeed);
    const float labelWidth = ImGui::CalcTextSize(top.c_str()).x;
    const ImVec2 boxMax = plotPos + ImVec2(plotSize.x - 10.0f * s, 14.0f * s + 2.5f * font + k_speedBands * bandHeight);
    const ImVec2 boxMin = ImVec2(boxMax.x - (barWidth + labelWidth + 22.0f * s), plotPos.y + 10.0f * s);
    dl->AddRectFilled(boxMin, boxMax, IM_COL32(15, 17, 24, 190), 4.0f * s);
    dl->AddText(boxMin + ImVec2(8.0f * s, 4.0f * s), k_hintColor, "|V|");

    const float barX = boxMin.x + 8.0f * s;
    const float barBottom = boxMax.y - 0.5f * font - 4.0f * s;
    for (int band = 0; band < k_speedBands; ++band) {
        const float y1 = barBottom - band * bandHeight;
        ImVec4 c = ImPlot::SampleColormap((band + 0.5f) / k_speedBands, ImPlotColormap_Viridis);
        dl->AddRectFilled({barX, y1 - bandHeight}, {barX + barWidth, y1}, ImGui::ColorConvertFloat4ToU32(c));
    }
    const auto label = [&](float y, const std::string& text) {
        dl->AddText({barX + barWidth + 6.0f * s, y - 0.5f * font}, k_hintColor, text.c_str());
    };
    label(barBottom, "0");
    label(barBottom - 0.5f * k_speedBands * bandHeight, std::format("{:.3g}", 0.5 * map.maxSpeed));
    label(barBottom - k_speedBands * bandHeight, top);
}

void drawStreamlines(ImDrawList* dl, const StreamlineSet& set, bool arrows, ImU32 colour = k_streamlineColor,
                     float width = 1.2f) {
    const float scale = uiScale();
    std::vector<ImVec2> pixels;
    for (const std::vector<Vec2>& line : set.lines) {
        pixels.resize(line.size());
        std::ranges::transform(line, pixels.begin(), toPixels);
        dl->AddPolyline(pixels.data(), static_cast<int>(pixels.size()), colour, ImDrawFlags_None, width * scale);
    }
    if (!arrows) return;
    const float size = 8.0f * scale;
    for (const Arrow& a : set.arrows) {
        const ImVec2 dir = normalized({static_cast<float>(a.direction.x), static_cast<float>(-a.direction.y)});
        drawArrowHead(dl, toPixels(a.position) + dir * (0.5f * size), dir, size, k_arrowColor);
    }
}

// Marker of one flow element or wall. `alpha` fades wall images.
void drawMarker(ImDrawList* dl, const SceneElement& element, const Rect& limits, float alpha) {
    const float s = uiScale();
    if (const auto* source = std::get_if<SourceSink>(&element)) {
        const ImVec2 c = toPixels(source->position);
        const float r = 6.0f * s;
        dl->AddCircleFilled(c, r, fadeColor(source->strength >= 0.0 ? k_sourceColor : k_sinkColor, alpha));
        const ImU32 ink = fadeColor(IM_COL32(20, 20, 25, 255), alpha);
        dl->AddLine({c.x - 0.55f * r, c.y}, {c.x + 0.55f * r, c.y}, ink, 1.6f * s);
        if (source->strength >= 0.0) dl->AddLine({c.x, c.y - 0.55f * r}, {c.x, c.y + 0.55f * r}, ink, 1.6f * s);
    } else if (const auto* vortex = std::get_if<Vortex>(&element)) {
        drawVortexGlyph(dl, toPixels(vortex->position), 7.0f * s, vortex->circulation >= 0.0, fadeColor(k_vortexColor, alpha));
    } else if (const auto* doublet = std::get_if<Doublet>(&element)) {
        drawDoubletGlyph(dl, toPixels(doublet->position), 5.0f * s, screenDirection(doublet->angle), fadeColor(k_doubletColor, alpha));
    } else if (const auto* sector = std::get_if<SectorFlow>(&element)) {
        const ImU32 colour = fadeColor(k_sectorColor, alpha);
        for (const double wall : {sector->wall1, sector->wall2}) {
            double t0 = 0.0;
            double t1 = 1e300;
            const Vec2 dir{std::cos(wall), std::sin(wall)};
            if (clipLine(sector->vertex, dir, limits, t0, t1)) {
                dl->AddLine(toPixels(sector->vertex + dir * t0), toPixels(sector->vertex + dir * t1), colour, 3.0f * s);
            }
        }
        dl->AddCircleFilled(toPixels(sector->vertex), 4.5f * s, colour);
    } else if (const auto* wall = std::get_if<SymmetryWall>(&element)) {
        double t0 = -1e300;
        double t1 = 1e300;
        if (clipLine(wall->point, wall->direction(), limits, t0, t1)) {
            const ImVec2 a = toPixels(wall->point + wall->direction() * t0);
            const ImVec2 b = toPixels(wall->point + wall->direction() * t1);
            dl->AddLine(a, b, fadeColor(k_wallColor, 0.25f * alpha), 7.0f * s);
            dl->AddLine(a, b, fadeColor(k_wallColor, alpha), 2.5f * s);
        }
        dl->AddCircleFilled(toPixels(wall->point), 3.0f * s, fadeColor(k_wallColor, alpha));
    }
}

// Uniform flows are drawn as labelled arrows stacked in the top-left corner of the plot.
ImVec2 uniformAnchor(ImVec2 plotPos, int slot) {
    const float s = uiScale();
    return plotPos + ImVec2(34.0f * s, (30.0f + 30.0f * slot) * s);
}

void drawUniformMarker(ImDrawList* dl, const UniformFlow& flow, ImVec2 anchor, std::size_t number) {
    const float s = uiScale();
    const ImVec2 dir = screenDirection(flow.angle);
    const ImVec2 tail = anchor - dir * (14.0f * s);
    const ImVec2 tip = anchor + dir * (14.0f * s);
    dl->AddCircleFilled(anchor, 17.0f * s, IM_COL32(15, 17, 24, 170));
    dl->AddLine(tail, tip - dir * (5.0f * s), k_uniformColor, 2.2f * s);
    drawArrowHead(dl, tip, dir, 8.0f * s, k_uniformColor);
    const std::string label = "#" + std::to_string(number) + "  " + describe(flow);
    dl->AddText(anchor + ImVec2(22.0f * s, -0.5f * ImGui::GetFontSize()), k_uniformColor, label.c_str());
}

// Screen distance from the mouse to an element's marker (walls: to the line).
float hitDistance(const SceneElement& element, ImVec2 mouse, const Rect& limits) {
    if (const auto* e = std::get_if<SymmetryWall>(&element)) {
        double t0 = -1e300;
        double t1 = 1e300;
        if (!clipLine(e->point, e->direction(), limits, t0, t1)) return 1e30f;
        return distanceToSegment(mouse, toPixels(e->point + e->direction() * t0),
                                 toPixels(e->point + e->direction() * t1));
    }
    Vec2 centre;
    if (const auto* e = std::get_if<SourceSink>(&element)) centre = e->position;
    if (const auto* e = std::get_if<Vortex>(&element)) centre = e->position;
    if (const auto* e = std::get_if<Doublet>(&element)) centre = e->position;
    if (const auto* e = std::get_if<SectorFlow>(&element)) centre = e->vertex;
    const ImVec2 d = toPixels(centre) - mouse;
    return std::sqrt(d.x * d.x + d.y * d.y);
}

} // namespace

PlotViewResult PlotView::draw(ImVec2 pos, ImVec2 size, const Scene& scene, const FlowModel& model,
                              const PlotLayers& layers) {
    const ParticleLayer& particles = layers.particles;
    PlotViewResult result;
    result.limits = scene.plot.view;

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("##plot-window", nullptr, flags);
    ImGui::PopStyleVar(2);

    constexpr ImPlotFlags plotFlags = ImPlotFlags_Equal | ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                                      ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText;
    if (ImPlot::BeginPlot("##flow", ImVec2(-1.0f, -1.0f), plotFlags)) {
        ImPlot::SetupAxes("x", "y");
        const Rect& v = m_pendingView ? *m_pendingView : scene.plot.view;
        ImPlot::SetupAxesLimits(v.min.x, v.max.x, v.min.y, v.max.y, m_pendingView ? ImPlotCond_Always : ImPlotCond_Once);
        m_pendingView.reset();
        ImPlot::SetupFinish();

        const ImPlotRect limits = ImPlot::GetPlotLimits();
        result.limits = {{limits.X.Min, limits.Y.Min}, {limits.X.Max, limits.Y.Max}};
        result.pixelSize = ImPlot::GetPlotSize();
        const ImVec2 plotPos = ImPlot::GetPlotPos();
        ImDrawList* dl = ImPlot::GetPlotDrawList();
        ImPlot::PushPlotClipRect();

        drawSpeedBands(dl, layers.speed);

        const bool hasSector = std::ranges::any_of(
            model.field.elements, [](const FlowElement& e) { return std::holds_alternative<SectorFlow>(e); });
        if (hasSector) drawMask(dl, model.field, plotPos, result.pixelSize);

        if (particles.texture != 0) {
            // The layer was rendered with exactly these limits; GL rows start at the bottom (y min).
            if (particles.beginBlend != nullptr) dl->AddCallback(particles.beginBlend, nullptr);
            // Premultiplied colour: a grey tint scales colour and alpha together, i.e. fades the layer
            // so the streamlines stay the main feature.
            dl->AddImage(ImTextureRef(particles.texture), plotPos, plotPos + result.pixelSize, ImVec2(0.0f, 1.0f),
                         ImVec2(1.0f, 0.0f), k_particleTint);
            if (particles.beginBlend != nullptr) dl->AddCallback(ImGui::GetPlatformIO().DrawCallback_ResetRenderState, nullptr);
        }

        drawStreamlines(dl, layers.equipotentials, false, k_equipotentialColor, 1.0f);
        drawStreamlines(dl, layers.streamlines, scene.plot.showArrows);

        if (scene.plot.showImages) {
            for (const FlowElement& image : model.images) {
                std::visit([&](const auto& e) { drawMarker(dl, SceneElement{e}, result.limits, k_imageAlpha); }, image);
            }
        }

        // Originals, remembering the one under the mouse.
        const bool hovered = ImPlot::IsPlotHovered();
        const ImVec2 mouse = ImGui::GetMousePos();
        std::optional<std::size_t> hit;
        float hitDist = k_hitRadiusPx * uiScale();
        int uniformSlot = 0;
        for (std::size_t i = 0; i < scene.elements.size(); ++i) {
            const SceneElement& element = scene.elements[i];
            float d = 0.0f;
            if (const auto* uniform = std::get_if<UniformFlow>(&element)) {
                const ImVec2 anchor = uniformAnchor(plotPos, uniformSlot++);
                drawUniformMarker(dl, *uniform, anchor, i + 1);
                const ImVec2 offset = anchor - mouse;
                d = std::sqrt(offset.x * offset.x + offset.y * offset.y) * 0.5f; // generous target
            } else {
                drawMarker(dl, element, result.limits, 1.0f);
                d = hitDistance(element, mouse, result.limits);
            }
            if (hovered && d < hitDist) {
                hitDist = d;
                hit = i;
            }
        }

        drawSpeedScale(dl, layers.speed, plotPos, result.pixelSize);

        if (scene.elements.empty()) {
            const char* hint = "Add flow elements with the toolbar on the left.";
            const ImVec2 textSize = ImGui::CalcTextSize(hint);
            dl->AddText(plotPos + (result.pixelSize - textSize) * 0.5f, k_hintColor, hint);
        }

        if (hit) {
            const SceneElement& element = scene.elements[*hit];
            ImVec2 centre = mouse;
            if (!std::holds_alternative<SymmetryWall>(element) && !std::holds_alternative<UniformFlow>(element)) {
                std::visit(
                    [&](const auto& e) {
                        using T = std::decay_t<decltype(e)>;
                        if constexpr (std::is_same_v<T, SectorFlow>) centre = toPixels(e.vertex);
                        else if constexpr (requires { e.position; }) centre = toPixels(e.position);
                    },
                    element);
                dl->AddCircle(centre, 11.0f * uiScale(), k_highlightColor, 0, 1.5f * uiScale());
            }
            ImGui::SetTooltip("#%zu  %s\n%s\nDouble-click to edit.", *hit + 1, kindName(kindOf(element)),
                              describe(element).c_str());
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) result.editRequest = hit;
        }

        ImPlot::PopPlotClipRect();
        if (hovered) {
            const ImPlotPoint m = ImPlot::GetPlotMousePos();
            result.mouse = Vec2{m.x, m.y};
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
    return result;
}

} // namespace pfl
