#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui/toolbar.hpp"

#include "ui/style.hpp"

namespace pfl {
namespace {

constexpr float k_buttonSize = 44.0f;
constexpr float k_padding = 6.0f;

} // namespace

float toolbarWidth() { return (k_buttonSize + 2.0f * k_padding) * uiScale(); }

std::optional<ElementKind> drawToolbar(ImVec2 pos, ImVec2 size) {
    std::optional<ElementKind> clicked;
    const float s = uiScale();
    const float button = k_buttonSize * s;

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_padding * s, k_padding * s));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, k_padding * s));
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("##toolbar", nullptr, flags);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (const ElementKind kind : k_elementKinds) {
        ImGui::PushID(static_cast<int>(kind));
        if (ImGui::InvisibleButton("##tool", ImVec2(button, button))) clicked = kind;
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const ImGuiCol background = ImGui::IsItemActive()    ? ImGuiCol_ButtonActive
                                    : ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered
                                                             : ImGuiCol_Button;
        dl->AddRectFilled(min, max, ImGui::GetColorU32(background), 6.0f * s);
        drawToolIcon(dl, (min + max) * 0.5f, 0.78f * button, kind);
        ImGui::SetItemTooltip("%s", kindName(kind));
        ImGui::PopID();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    return clicked;
}

} // namespace pfl
