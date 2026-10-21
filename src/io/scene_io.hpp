// scene_io.hpp: save/load flow configurations as JSON (future).
//
// Will serialise the flow field (elements, walls, view, streamline seeds) to a versioned JSON
// document and read it back with validation, for the files in examples/.
#pragma once

#include <string>

namespace pfl {

// Placeholder: the header every scene file will start with, e.g. {"format":"pfl-scene","version":1}.
std::string sceneFileHeaderJson();

} // namespace pfl
