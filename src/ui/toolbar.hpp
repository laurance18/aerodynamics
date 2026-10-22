// toolbar.hpp: left-hand icon toolbar with one button per element type (uniform flow, source/sink,
// vortex, doublet, sector flow, symmetry wall) and, below them, the dye rake tool. Hovering a button
// shows its name; clicking an element asks the caller to open the parameter dialog for a new element
// of that type, clicking the rake tool asks it to start placing a rake on the plot.
#pragma once

#include "core/scene.hpp"

#include <imgui.h>

#include <optional>

namespace pfl {

struct ToolbarAction {
    std::optional<ElementKind> element;
    bool placeRake = false;
};

float toolbarWidth();

// `placingRake` highlights the rake tool while a rake is being placed.
ToolbarAction drawToolbar(ImVec2 pos, ImVec2 size, bool placingRake);

} // namespace pfl
