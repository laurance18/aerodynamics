// parameter_dialog.hpp: compact modal dialog for an element's parameters.
//
// Edits a pending copy of the element (strength, position, angles in degrees, ...); Apply commits
// it, Cancel or Esc throws it away. Invalid parameters disable Apply and say why. Legal but
// noteworthy results (coincident singularities, an element on a wall, ...) are listed as warnings
// before applying.
#pragma once

#include "core/scene.hpp"

#include <cstddef>
#include <optional>

namespace pfl {

struct ElementEdit {
    std::optional<std::size_t> index; // element to replace; nullopt appends a new one
    SceneElement element;
};

// Returns `scene` with `edit` applied.
Scene applyEdit(Scene scene, const ElementEdit& edit);

class ParameterDialog {
public:
    void openNew(SceneElement element);
    void openEdit(std::size_t index, SceneElement element);

    // Draws the dialog while it is open; returns the edit when the user presses Apply.
    std::optional<ElementEdit> draw(const Scene& scene);

private:
    void open(std::optional<std::size_t> index, SceneElement element);

    SceneElement m_pending;
    std::optional<std::size_t> m_index;
    bool m_openRequested = false;
};

} // namespace pfl
