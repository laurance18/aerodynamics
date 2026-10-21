#include "ui/main_menu.hpp"

#include <imgui.h>

namespace pfl {

MainMenuActions drawMainMenu(bool canReloadShaders) {
    MainMenuActions actions;
    if (canReloadShaders && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, ImGuiInputFlags_RouteGlobal)) {
        actions.reloadShaders = true;
    }

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (canReloadShaders && ImGui::MenuItem("Reload shaders", "Ctrl+R")) actions.reloadShaders = true;
            if (canReloadShaders) ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) actions.quit = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    return actions;
}

} // namespace pfl
