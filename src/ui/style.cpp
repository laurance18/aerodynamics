#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/style.hpp"

#include <cmath>
#include <numbers>

namespace pfl {
namespace {

constexpr float k_pi = std::numbers::pi_v<float>;

ImVec2 polar(float r, float angle) { return {r * std::cos(angle), r * std::sin(angle)}; }

} // namespace

ImU32 fadeColor(ImU32 colour, float alpha) {
    const auto a = static_cast<ImU32>(static_cast<float>((colour >> IM_COL32_A_SHIFT) & 0xFF) * alpha);
    return (colour & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

void drawArrowHead(ImDrawList* dl, ImVec2 tip, ImVec2 dir, float size, ImU32 colour) {
    const ImVec2 normal{-dir.y, dir.x};
    const ImVec2 base = tip - dir * size;
    dl->AddTriangleFilled(tip, base + normal * (0.45f * size), base - normal * (0.45f * size), colour);
}

void drawVortexGlyph(ImDrawList* dl, ImVec2 centre, float radius, bool clockwise, ImU32 colour) {
    const float s = uiScale();
    // Screen angles grow clockwise because screen y points down.
    const float a0 = 0.6f;
    const float a1 = a0 + 1.6f * k_pi;
    dl->PathArcTo(centre, radius, a0, a1, 24);
    dl->PathStroke(colour, ImDrawFlags_None, 2.0f * s);
    const float end = clockwise ? a1 : a0;
    const ImVec2 tangent = polar(1.0f, end + 0.5f * k_pi) * (clockwise ? 1.0f : -1.0f);
    drawArrowHead(dl, centre + polar(radius, end) + tangent * (3.0f * s), tangent, 6.0f * s, colour);
    dl->AddCircleFilled(centre, 2.0f * s, colour);
}

void drawDoubletGlyph(ImDrawList* dl, ImVec2 centre, float radius, ImVec2 axis, ImU32 colour) {
    const float s = uiScale();
    const ImVec2 normal{-axis.y, axis.x};
    dl->AddCircle(centre + normal * radius, radius, colour, 0, 1.5f * s);
    dl->AddCircle(centre - normal * radius, radius, colour, 0, 1.5f * s);
    dl->AddLine(centre + axis * (1.4f * radius), centre - axis * (1.4f * radius), colour, 1.8f * s);
    drawArrowHead(dl, centre - axis * (2.2f * radius), -axis, 0.9f * radius + 2.0f * s, colour);
}

void drawToolIcon(ImDrawList* dl, ImVec2 c, float size, ElementKind kind) {
    const float s = uiScale();
    const float h = 0.5f * size;
    switch (kind) {
    case ElementKind::Uniform:
        for (const float dy : {-0.45f, 0.0f, 0.45f}) {
            const ImVec2 a = c + ImVec2(-0.7f * h, dy * h);
            const ImVec2 b = c + ImVec2(0.55f * h, dy * h);
            dl->AddLine(a, b, k_uniformColor, 1.8f * s);
            drawArrowHead(dl, b + ImVec2(0.25f * h, 0.0f), ImVec2(1.0f, 0.0f), 0.3f * h, k_uniformColor);
        }
        break;
    case ElementKind::SourceSink:
        for (int k = 0; k < 8; ++k) {
            const float a = k * 0.25f * k_pi;
            const ImVec2 dir = polar(1.0f, a);
            dl->AddLine(c + dir * (0.3f * h), c + dir * (0.7f * h), k_sourceColor, 1.5f * s);
            drawArrowHead(dl, c + dir * (0.9f * h), dir, 0.22f * h, k_sourceColor);
        }
        dl->AddCircleFilled(c, 0.2f * h, k_sourceColor);
        break;
    case ElementKind::Vortex: drawVortexGlyph(dl, c, 0.62f * h, true, k_vortexColor); break;
    case ElementKind::Doublet: drawDoubletGlyph(dl, c, 0.36f * h, ImVec2(1.0f, 0.0f), k_doubletColor); break;
    case ElementKind::Sector: {
        // 90-degree corner with two hyperbolic streamlines x y = const.
        const ImVec2 o = c + ImVec2(-0.75f * h, 0.75f * h);
        dl->AddLine(o, o + ImVec2(1.6f * h, 0.0f), k_sectorColor, 2.2f * s);
        dl->AddLine(o, o - ImVec2(0.0f, 1.6f * h), k_sectorColor, 2.2f * s);
        for (const float k : {0.18f, 0.5f}) {
            for (int i = 0; i < 12; ++i) {
                const float x = (k / 1.5f) + (1.5f - k / 1.5f) * i / 11.0f;
                dl->PathLineTo(o + ImVec2(x * h, -(k / x) * h));
            }
            dl->PathStroke(k_sectorColor, ImDrawFlags_None, 1.4f * s);
        }
        break;
    }
    case ElementKind::Wall: {
        const ImVec2 a = c + ImVec2(-0.8f * h, 0.1f * h);
        const ImVec2 b = c + ImVec2(0.8f * h, 0.1f * h);
        dl->AddLine(a, b, k_wallColor, 2.5f * s);
        for (int i = 0; i < 6; ++i) {
            const ImVec2 p = a + (b - a) * ((i + 0.5f) / 6.0f);
            dl->AddLine(p, p + ImVec2(-0.25f * h, 0.35f * h), fadeColor(k_wallColor, 0.7f), 1.2f * s);
        }
        // A streamline above the wall, mirrored by a faint one below.
        dl->AddLine(c + ImVec2(-0.8f * h, -0.45f * h), c + ImVec2(0.8f * h, -0.45f * h), k_uniformColor, 1.2f * s);
        drawArrowHead(dl, c + ImVec2(0.85f * h, -0.45f * h), ImVec2(1.0f, 0.0f), 0.25f * h, k_uniformColor);
        break;
    }
    }
}

} // namespace pfl
