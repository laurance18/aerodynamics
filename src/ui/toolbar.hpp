// toolbar.hpp: left-hand icon toolbar with one button per element type (uniform flow, source/sink,
// vortex, doublet, sector flow, symmetry wall). Hovering a button shows its name; clicking it asks
// the caller to open the parameter dialog for a new element of that type.
#pragma once

#include "core/scene.hpp"

#include <imgui.h>

#include <optional>

namespace pfl {

float toolbarWidth();

std::optional<ElementKind> drawToolbar(ImVec2 pos, ImVec2 size);

} // namespace pfl
