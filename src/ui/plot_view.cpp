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
constexpr ImU32 k_surfaceCasingColor = IM_COL32(235, 235, 235, 235);
constexpr float k_imageAlpha = 0.35f;
constexpr float k_maskCellPx = 6.0f;
constexpr float k_hitRadiusPx = 10.0f;
constexpr float k_speedAlpha = 0.6f;
constexpr float k_pressureAlpha = 0.75f;

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

// Colour of band `band` (0 lowest): Viridis for |V|, the diverging pressure scale for Cp.
ImU32 bandColor(const ContourMap& map, int band, float alpha) {
    if (map.quantity == Background::Speed) {
        ImVec4 c = ImPlot::SampleColormap((band + 0.5f) / map.bands, ImPlotColormap_Viridis);
        c.w = alpha;
        return ImGui::ColorConvertFloat4ToU32(c);
    }
    return pressureColor(map.low + (band + 0.5) * (map.high - map.low) / map.bands, map.low, alpha);
}

// Filled bands. Cells in the same band are merged into horizontal runs.
void drawContourBands(ImDrawList* dl, const ContourMap& map) {
    const ScalarGrid& grid = map.values;
    if (map.quantity == Background::None || map.bands <= 0 || grid.columns <= 0 || grid.rows <= 0) return;
    const float alpha = map.quantity == Background::Speed ? k_speedAlpha : k_pressureAlpha;
    const Vec2 cell{grid.bounds.size().x / grid.columns, grid.bounds.size().y / grid.rows};
    std::vector<float> xs(static_cast<std::size_t>(grid.columns) + 1);
    for (int i = 0; i <= grid.columns; ++i) xs[static_cast<std::size_t>(i)] = toPixels(grid.bounds.min + Vec2{cell.x * i, 0.0}).x;

    const auto bandOf = [&](int i, int j) {
        const double value = grid.at(i, j);
        if (std::isnan(value)) return -1; // outside the domain: the mask covers it
        // Clamp before converting: the value is infinite at singular points.
        return static_cast<int>(std::clamp((value - map.low) / (map.high - map.low) * map.bands, 0.0, map.bands - 0.5));
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
                                  bandColor(map, runBand, alpha));
            }
            runStart = i;
            runBand = band;
        }
    }
}

// Colour scale for the bands in the top-right corner of the plot.
void drawContourScale(ImDrawList* dl, const ContourMap& map, ImVec2 plotPos, ImVec2 plotSize) {
    if (map.quantity == Background::None) return;
    const float s = uiScale();
    const float font = ImGui::GetFontSize();
    const bool speed = map.quantity == Background::Speed;
    if (map.bands <= 0) {
        if (speed) return;
        const char* note = "Cp needs a free stream: add a uniform flow.";
        const ImVec2 textSize = ImGui::CalcTextSize(note);
        const ImVec2 boxMax = plotPos + ImVec2(plotSize.x - 10.0f * s, 10.0f * s + textSize.y + 8.0f * s);
        const ImVec2 boxMin{boxMax.x - textSize.x - 16.0f * s, plotPos.y + 10.0f * s};
        dl->AddRectFilled(boxMin, boxMax, IM_COL32(15, 17, 24, 190), 4.0f * s);
        dl->AddText(boxMin + ImVec2(8.0f * s, 4.0f * s), k_hintColor, note);
        return;
    }

    std::vector<std::pair<double, std::string>> labels;
    if (speed) {
        const double middle = 0.5 * (map.low + map.high);
        labels = {{map.low, "0"}, {middle, std::format("{:.3g}", middle)}, {map.high, std::format("≥ {:.3g}", map.high)}};
    } else {
        labels = {{map.low, std::format("≤ {:.3g}", map.low)}, {0.0, "0"}, {map.high, std::format("{:.3g}", map.high)}};
    }
    float labelWidth = 0.0f;
    for (const auto& [value, text] : labels) labelWidth = std::max(labelWidth, ImGui::CalcTextSize(text.c_str()).x);

    const float barWidth = 14.0f * s;
    const float bandHeight = std::min(16.0f, 160.0f / static_cast<float>(map.bands)) * s;
    const float barHeight = static_cast<float>(map.bands) * bandHeight;
    const ImVec2 boxMax = plotPos + ImVec2(plotSize.x - 10.0f * s, 14.0f * s + 2.5f * font + barHeight);
    const ImVec2 boxMin = ImVec2(boxMax.x - (barWidth + labelWidth + 22.0f * s), plotPos.y + 10.0f * s);
    dl->AddRectFilled(boxMin, boxMax, IM_COL32(15, 17, 24, 190), 4.0f * s);
    dl->AddText(boxMin + ImVec2(8.0f * s, 4.0f * s), k_hintColor, speed ? "|V|" : "Cp");

    const float barX = boxMin.x + 8.0f * s;
    const float barBottom = boxMax.y - 0.5f * font - 4.0f * s;
    for (int band = 0; band < map.bands; ++band) {
        const float y1 = barBottom - static_cast<float>(band) * bandHeight;
        dl->AddRectFilled({barX, y1 - bandHeight}, {barX + barWidth, y1}, bandColor(map, band, 1.0f));
    }
    for (const auto& [value, text] : labels) {
        const float y = barBottom - static_cast<float>((value - map.low) / (map.high - map.low)) * barHeight;
        dl->AddText({barX + barWidth + 6.0f * s, y - 0.5f * font}, k_hintColor, text.c_str());
    }
}

// Body surfaces coloured by Cp, with arrows normal to the surface: towards it where Cp > 0
// (pressure above the free-stream value pushes on the body) and away from it where Cp < 0 (suction).
// Arrow length is proportional to |Cp|, the largest |Cp| on any body getting the longest arrow.
void drawSurfaces(ImDrawList* dl, const SurfaceLayer& layer) {
    const float s = uiScale();
    const float minGap = 2.0f * s;        // drawn vertices at least this far apart
    const float arrowSpacing = 24.0f * s; // along the surface
    const float maxArrow = 48.0f * s;
    double largest = 0.0;
    for (const BodySurface& body : layer.bodies) {
        for (const double cp : body.cp) largest = std::max(largest, std::abs(cp));
    }
    const float pixelsPerCp = largest > 0.0 ? maxArrow / static_cast<float>(largest) : 0.0f;
    std::vector<std::size_t> kept;
    std::vector<ImVec2> pixels;
    for (std::size_t b = 0; b < layer.bodies.size(); ++b) {
        const BodySurface& body = layer.bodies[b];
        if (body.points.size() < 2) continue;
        kept.assign(1, 0);
        pixels.assign(1, toPixels(body.points.front()));
        for (std::size_t k = 1; k < body.points.size(); ++k) {
            const ImVec2 p = toPixels(body.points[k]);
            const ImVec2 d = p - pixels.back();
            if (d.x * d.x + d.y * d.y < minGap * minGap && k + 1 < body.points.size()) continue;
            kept.push_back(k);
            pixels.push_back(p);
        }

        // A light casing keeps the surface visible on the Cp bands, which share its colours.
        dl->AddPolyline(pixels.data(), static_cast<int>(pixels.size()), k_surfaceCasingColor, ImDrawFlags_None, 6.5f * s);
        float along = 0.5f * arrowSpacing;
        for (std::size_t i = 0; i + 1 < kept.size(); ++i) {
            const double cp = 0.5 * (body.cp[kept[i]] + body.cp[kept[i + 1]]);
            const ImU32 colour = pressureColor(cp, layer.lowestCp);
            dl->AddLine(pixels[i], pixels[i + 1], colour, 3.5f * s);

            const ImVec2 d = pixels[i + 1] - pixels[i];
            along += std::sqrt(d.x * d.x + d.y * d.y);
            if (along < arrowSpacing) continue;
            along = 0.0f;
            const float size = static_cast<float>(std::abs(cp)) * pixelsPerCp;
            if (size < 4.0f * s) continue;
            // perp of the world direction is the outward normal; screen y points down.
            const Vec2 n = perp(body.points[kept[i + 1]] - body.points[kept[i]]);
            const ImVec2 out = normalized({static_cast<float>(n.x), static_cast<float>(-n.y)});
            const ImVec2 base = pixels[i] + out * (3.0f * s);
            const ImVec2 tip = cp > 0.0 ? base : base + out * size;
            const ImVec2 tail = cp > 0.0 ? base + out * size : base;
            const ImVec2 dir = cp > 0.0 ? out * -1.0f : out;
            // Full red or blue: the length shows the magnitude.
            const ImU32 arrowColour = pressureColor(cp > 0.0 ? 1.0 : layer.lowestCp, layer.lowestCp);
            dl->AddLine(tail, tip - dir * (4.0f * s), arrowColour, 1.5f * s);
            drawArrowHead(dl, tip, dir, 6.0f * s, arrowColour);
        }

        if (layer.bodies.size() > 1) {
            // Number the bodies (as in the Cp panel) above their highest point.
            const ImVec2 top = *std::ranges::min_element(pixels, {}, &ImVec2::y);
            const std::string label = std::to_string(b + 1);
            const float font = ImGui::GetFontSize();
            const ImVec2 at = top - ImVec2(0.0f, 10.0f * s + font);
            dl->AddCircleFilled(at + ImVec2(-6.0f * s, 0.5f * font), 4.0f * s,
                                b < k_bodyColors.size() ? k_bodyColors[b] : k_hintColor);
            dl->AddText(at, k_hintColor, label.c_str());
        }
    }
    if (layer.marker) {
        const ImVec2 p = toPixels(*layer.marker);
        dl->AddCircle(p, 7.0f * s, k_highlightColor, 0, 2.0f * s);
        dl->AddCircleFilled(p, 2.5f * s, k_highlightColor);
    }
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

        drawContourBands(dl, layers.contours);

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
        drawSurfaces(dl, layers.surfaces);

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

        drawContourScale(dl, layers.contours, plotPos, result.pixelSize);

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
