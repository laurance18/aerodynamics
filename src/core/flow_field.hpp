// flow_field.hpp: superposition of flow elements.
//
// Holds the list of active elements and evaluates the total complex velocity and potential by
// summing each element's W and F. This is the single source of truth that the UI edits and the
// renderers and streamline integrator read.
//
// Singularities: an element's term is skipped within singularRadius of its singular point (see
// singularPoint()), so the total stays finite there. Callers that should stop or respawn near a
// singularity (streamlines, particles) ask nearSingularity() with their own radius.
//
// Domain: sector flows are defined only inside their wedge, so the admissible domain is the
// intersection of all sector wedges. Outside it (inDomain() false) the field is undefined and must
// be masked. Other elements are superposed unrestricted and do not keep the sector walls as
// streamlines. Wall images (image_system.hpp) are not included yet.
#pragma once

#include "core/flow_element.hpp"
#include "core/vec2.hpp"

#include <vector>

namespace pfl {

struct FlowField {
    std::vector<FlowElement> elements;
    double singularRadius = 1e-9;

    Complex complexVelocity(Vec2 p) const;
    Complex complexPotential(Vec2 p) const;
    Vec2 velocity(Vec2 p) const { return velocityFromW(complexVelocity(p)); }

    bool inDomain(Vec2 p) const;
    bool nearSingularity(Vec2 p, double radius) const;
};

} // namespace pfl
