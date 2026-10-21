#include "ui/demo_panel.hpp"

#include <imgui.h>
#include <implot.h>

namespace pfl {
namespace {

constexpr const char* k_strengthPopup = "Flow parameters";

} // namespace

float DemoPanel::draw(DemoSettings& settings, std::size_t particleCount) {
    const ImGuiIO& io = ImGui::GetIO();
    recordFrameTime(1000.0f * io.DeltaTime);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = 300.0f * ImGui::GetStyle().FontScaleDpi;
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(width, viewport->WorkSize.y));
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoDocking;
    ImGui::Begin("Demo", nullptr, flags);

    ImGui::SeparatorText("Performance");
    ImGui::Text("%.1f FPS (%.2f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
    if (ImPlot::BeginPlot("##frametime", ImVec2(-1.0f, 110.0f * ImGui::GetStyle().FontScaleDpi),
                          ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_None);
        ImPlot::SetupAxesLimits(0, static_cast<double>(m_frameMs.size()), 0, 40, ImPlotCond_Always);
        ImPlotSpec spec;
        spec.Offset = m_frameHead; // ring buffer: oldest sample first
        ImPlot::PlotLine("frame ms", m_frameMs.data(), static_cast<int>(m_frameMs.size()), 1.0, 0.0, spec);
        ImPlot::EndPlot();
    }

    ImGui::SeparatorText("Flow");
    ImGui::Text("Vortex strength S = %.2f", settings.flow.strength);
    ImGui::Text("Drift U = %.2f", settings.flow.drift);
    if (ImGui::Button("Vortex strength...", ImVec2(-1.0f, 0.0f))) {
        m_pending = settings.flow;
        ImGui::OpenPopup(k_strengthPopup);
    }
    ImGui::SetItemTooltip("Open a dialog to change the vortex strength S.\n"
                          "Changes only take effect when you press Apply.");
    drawStrengthDialog(settings);

    ImGui::SeparatorText("Particles");
    ImGui::Text("%zu tracer particles (CPU RK2)", particleCount);
    ImGui::SliderFloat("Point size", &settings.pointSize, 1.0f, 6.0f, "%.1f px");
    ImGui::Checkbox("Trails", &settings.trails);
    ImGui::BeginDisabled(!settings.trails);
    ImGui::SliderFloat("Fade", &settings.trailFade, 0.01f, 0.5f, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::EndDisabled();

    ImGui::End();
    return width;
}

void DemoPanel::drawStrengthDialog(DemoSettings& settings) {
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal(k_strengthPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    float strength = static_cast<float>(m_pending.strength);
    ImGui::SetNextItemWidth(260.0f * ImGui::GetStyle().FontScaleDpi);
    if (ImGui::SliderFloat("Strength", &strength, -5.0f, 5.0f, "%.2f")) m_pending.strength = strength;
    ImGui::TextDisabled("Positive: counter-clockwise. Current value: %.2f", settings.flow.strength);

    ImGui::Spacing();
    if (ImGui::Button("Apply", ImVec2(120.0f, 0.0f))) {
        settings.flow = m_pending;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void DemoPanel::recordFrameTime(float ms) {
    m_frameMs[static_cast<std::size_t>(m_frameHead)] = ms;
    m_frameHead = (m_frameHead + 1) % static_cast<int>(m_frameMs.size());
}

} // namespace pfl
