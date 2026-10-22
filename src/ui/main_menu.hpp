// main_menu.hpp: the application's main menu bar (File, Examples, Elements, View) and its shortcuts.
//
// File: New, Open, Save, Save As, the image and CSV exports, Clear All, Quit (and Reload shaders in
// Debug builds). Examples: the built-in example scenes (io/examples.hpp), each loaded with one click.
// Elements: see elements_menu.hpp. View: reset the view and the plot settings that
// are saved with the scene (streamline density, arrows, wall images, equipotentials, |V| or Cp
// background, surface pressure, particles), edited in place.
#pragma once

#include "core/scene.hpp"
#include "ui/elements_menu.hpp"

#include <cstddef>
#include <optional>

namespace pfl {

struct MainMenuActions {
    bool newScene = false;
    bool open = false;
    bool save = false;
    bool saveAs = false;
    bool exportImage = false;
    bool exportField = false;
    bool exportLines = false;
    bool exportSurface = false;
    bool clearAll = false;
    bool quit = false;
    bool reloadShaders = false;
    bool resetView = false;
    bool plotChanged = false; // a View setting in `plot` was edited
    std::optional<std::size_t> example; // index into exampleScenes()
    ElementsMenuAction elements;
};

// Draws the menu bar and handles its keyboard shortcuts. "Reload shaders" (Ctrl+R) is only
// offered when `canReloadShaders` is true (Debug builds reading shaders from disk).
MainMenuActions drawMainMenu(const Scene& scene, PlotSettings& plot, bool canReloadShaders);

// "Discard unsaved changes?" confirmation. Open it, then draw it every frame; it returns true
// (discard) or false (cancel) once the user has chosen.
void openDiscardChangesDialog();
std::optional<bool> drawDiscardChangesDialog();

} // namespace pfl
