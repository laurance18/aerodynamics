// elements_menu.hpp: the "Elements" pull-down menu.
//
// Lists every element currently in the scene with its parameters. Each entry opens a submenu to
// edit or delete that element; "Delete all" empties the list.
#pragma once

#include "core/scene.hpp"

#include <cstddef>
#include <optional>

namespace pfl {

struct ElementsMenuAction {
    std::optional<std::size_t> edit;
    std::optional<std::size_t> remove;
    bool removeAll = false;
};

// Draws the menu; call between BeginMainMenuBar() and EndMainMenuBar().
ElementsMenuAction drawElementsMenu(const Scene& scene);

} // namespace pfl
