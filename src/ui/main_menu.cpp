#include "ui/main_menu.hpp"

#include "ui/style.hpp"

#include <imgui.h>

namespace pfl {
namespace {

constexpr const char* k_discardPopup = "Unsaved changes";

void drawViewMenu(PlotSettings& plot, MainMenuActions& actions) {
    if (!ImGui::BeginMenu("View")) return;
    if (ImGui::MenuItem("Reset view")) actions.resetView = true;
    ImGui::SetItemTooltip("Show the default region -4 < x < 4, -2.5 < y < 2.5.\n"
                          "Scroll to zoom and drag to pan the plot.");
    ImGui::Separator();

    ImGui::SetNextItemWidth(180.0f * uiScale());
    actions.plotChanged |= ImGui::SliderInt("Streamline density", &plot.streamlineDensity, 5, 80);
    ImGui::SetItemTooltip("Number of streamlines across the height of the view.");
    actions.plotChanged |= ImGui::Checkbox("Direction arrows", &plot.showArrows);
    ImGui::BeginDisabled(!plot.showArrows);
    float spacing = static_cast<float>(plot.arrowSpacingPx);
    ImGui::SetNextItemWidth(180.0f * uiScale());
    if (ImGui::SliderFloat("Arrow spacing", &spacing, 40.0f, 400.0f, "%.0f px")) {
        plot.arrowSpacingPx = spacing;
        actions.plotChanged = true;
    }
    ImGui::EndDisabled();
    actions.plotChanged |= ImGui::Checkbox("Show wall images", &plot.showImages);
    ImGui::SetItemTooltip("Draw the mirrored sources, vortices and doublets that enforce the symmetry walls.");
    actions.plotChanged |= ImGui::Checkbox("Equipotential lines", &plot.showEquipotentials);
    ImGui::SetItemTooltip("Lines of constant velocity potential φ, crossing every streamline at right angles.");
    actions.plotChanged |= ImGui::Checkbox("Speed contours |V|", &plot.showSpeedContours);
    ImGui::SetItemTooltip("Filled bands of velocity magnitude, from 0 to twice the typical speed in the view.");
    actions.plotChanged |= ImGui::Checkbox("Tracer particles", &plot.showParticles);
    ImGui::BeginDisabled(!plot.showParticles);
    actions.plotChanged |= ImGui::Checkbox("Particle trails", &plot.particleTrails);
    ImGui::EndDisabled();
    ImGui::EndMenu();
}

} // namespace

MainMenuActions drawMainMenu(const Scene& scene, PlotSettings& plot, bool canReloadShaders) {
    MainMenuActions actions;
    constexpr ImGuiInputFlags route = ImGuiInputFlags_RouteGlobal;
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, route)) actions.newScene = true;
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, route)) actions.open = true;
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, route)) actions.save = true;
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, route)) actions.saveAs = true;
    if (canReloadShaders && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, route)) actions.reloadShaders = true;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) actions.newScene = true;
            ImGui::SetItemTooltip("Start an empty scene with default plot settings.");
            if (ImGui::MenuItem("Open...", "Ctrl+O")) actions.open = true;
            if (ImGui::MenuItem("Save", "Ctrl+S")) actions.save = true;
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) actions.saveAs = true;
            ImGui::Separator();
            const bool hasFlow = !scene.elements.empty();
            if (ImGui::MenuItem("Export image (PNG)...", nullptr, false, hasFlow)) actions.exportImage = true;
            ImGui::SetItemTooltip("Save the plot, axes included, as it is shown now.");
            if (ImGui::MenuItem("Export field data (CSV)...", nullptr, false, hasFlow)) actions.exportField = true;
            ImGui::SetItemTooltip("x, y, u, v, |V|, φ, ψ and Cp on a 201-column grid over the current view.");
            if (ImGui::MenuItem("Export lines (CSV)...", nullptr, false, hasFlow)) actions.exportLines = true;
            ImGui::SetItemTooltip("The points of every streamline and equipotential line shown now.");
            ImGui::Separator();
            if (ImGui::MenuItem("Clear All", nullptr, false, !scene.elements.empty())) actions.clearAll = true;
            ImGui::SetItemTooltip("Remove every element but keep the view and plot settings.");
            ImGui::Separator();
            if (canReloadShaders && ImGui::MenuItem("Reload shaders", "Ctrl+R")) actions.reloadShaders = true;
            if (ImGui::MenuItem("Quit", "Alt+F4")) actions.quit = true;
            ImGui::EndMenu();
        }
        actions.elements = drawElementsMenu(scene);
        drawViewMenu(plot, actions);
        ImGui::EndMainMenuBar();
    }
    return actions;
}

void openDiscardChangesDialog() { ImGui::OpenPopup(k_discardPopup); }

std::optional<bool> drawDiscardChangesDialog() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal(k_discardPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return std::nullopt;

    std::optional<bool> choice;
    ImGui::TextUnformatted("The scene has unsaved changes. Discard them?");
    ImGui::Spacing();
    const ImVec2 buttonSize(120.0f * uiScale(), 0.0f);
    if (ImGui::Button("Discard", buttonSize)) choice = true;
    ImGui::SameLine();
    if (ImGui::Button("Cancel", buttonSize) || ImGui::IsKeyPressed(ImGuiKey_Escape)) choice = false;
    if (choice) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
    return choice;
}

} // namespace pfl
