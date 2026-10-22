#include "ui/elements_menu.hpp"

#include <imgui.h>

#include <format>
#include <string>

namespace pfl {

ElementsMenuAction drawElementsMenu(const Scene& scene) {
    ElementsMenuAction action;
    if (!ImGui::BeginMenu("Elements")) return action;

    if (scene.elements.empty()) {
        ImGui::TextDisabled("No elements yet. Add one with the toolbar on the left.");
    }
    for (std::size_t i = 0; i < scene.elements.size(); ++i) {
        const SceneElement& element = scene.elements[i];
        const std::string label =
            std::format("#{}  {}:  {}###element{}", i + 1, kindName(kindOf(element)), describe(element), i);
        if (ImGui::BeginMenu(label.c_str())) {
            if (ImGui::MenuItem("Edit...")) action.edit = i;
            if (ImGui::MenuItem("Delete")) action.remove = i;
            ImGui::EndMenu();
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Delete all", nullptr, false, !scene.elements.empty())) action.removeAll = true;
    const std::string rakes = std::format("Delete all dye rakes ({})", scene.rakes.size());
    if (ImGui::MenuItem(rakes.c_str(), nullptr, false, !scene.rakes.empty())) action.removeAllRakes = true;
    ImGui::SetItemTooltip("Dye rakes are placed with the rake tool at the bottom of the toolbar.\n"
                          "Right-click a rake for its options.");
    ImGui::EndMenu();
    return action;
}

} // namespace pfl
