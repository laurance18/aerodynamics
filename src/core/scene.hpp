// scene.hpp: what the user builds, edits and saves: the element list (flows and symmetry walls)
// plus the plot settings, and the flow model derived from it.
//
// buildFlowModel() turns a scene into the field that is actually evaluated: every flow element,
// the wall images of its sources, vortices and doublets, and its uniform flows reduced to what the
// walls allow (see image_system.hpp). The UI shows warnings for configurations that are legal but
// easy to misread (coincident singularities, elements on a wall, truncated images, ...).
#pragma once

#include "core/flow_element.hpp"
#include "core/flow_field.hpp"
#include "core/image_system.hpp"
#include "core/vec2.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pfl {

using SceneElement = std::variant<UniformFlow, SourceSink, Vortex, Doublet, SectorFlow, SymmetryWall>;

// One per SceneElement alternative, in the same order.
enum class ElementKind { Uniform, SourceSink, Vortex, Doublet, Sector, Wall };
inline constexpr std::array<ElementKind, 6> k_elementKinds = {ElementKind::Uniform, ElementKind::SourceSink,
                                                               ElementKind::Vortex,  ElementKind::Doublet,
                                                               ElementKind::Sector,  ElementKind::Wall};

inline ElementKind kindOf(const SceneElement& element) { return static_cast<ElementKind>(element.index()); }
const char* kindName(ElementKind kind);

// A new element with sensible default parameters, placed at `position` where that applies.
SceneElement defaultElement(ElementKind kind, Vec2 position);

// One-line parameter summary in UTF-8, e.g. "Λ = 6.283 at (1, 0)". Angles in degrees.
std::string describe(const SceneElement& element);

// Why the parameters are invalid, or nullopt if they are fine.
std::optional<std::string> sceneElementError(const SceneElement& element);

struct PlotSettings {
    Rect view{{-4.0, -2.5}, {4.0, 2.5}};
    int streamlineDensity = 24; // streamlines across the view height
    bool showArrows = true;
    double arrowSpacingPx = 110.0; // screen distance between direction arrows along a streamline
    bool showImages = true;        // draw the wall images of elements
    bool showParticles = true;
    bool particleTrails = true;
};

struct Scene {
    std::vector<SceneElement> elements;
    PlotSettings plot;
};

struct FlowModel {
    FlowField field;                  // what is evaluated: flows (uniform ones constrained) + images
    std::vector<FlowElement> images;  // the wall images on their own, for drawing
    std::vector<SymmetryWall> walls;
    bool imagesExact = true;
    double freeStreamSpeed = 0.0;     // |sum of the constrained uniform flows|; the Cp reference
};

FlowModel buildFlowModel(const Scene& scene);

// Human-readable notes about the scene (element numbers are 1-based, as in the Elements menu).
std::vector<std::string> sceneWarnings(const Scene& scene, const FlowModel& model);

// Numerical check that a wall is a streamline: velocity sampled along the part of the wall inside
// `region`, skipping points near singularities and outside the flow domain.
struct WallCheck {
    double maxNormalSpeed = 0.0; // largest |v . n|
    double maxSpeed = 0.0;       // largest |v|, for scale
    int samples = 0;
};
WallCheck checkWall(const FlowField& field, const SymmetryWall& wall, const Rect& region, int samples = 64);

} // namespace pfl
