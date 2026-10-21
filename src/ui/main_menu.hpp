// main_menu.hpp: the application's main menu bar.
#pragma once

namespace pfl {

struct MainMenuActions {
    bool quit = false;
    bool reloadShaders = false;
};

// Draws the menu bar and handles its keyboard shortcuts. "Reload shaders" (Ctrl+R) is only
// offered when `canReloadShaders` is true (Debug builds reading shaders from disk).
MainMenuActions drawMainMenu(bool canReloadShaders);

} // namespace pfl
