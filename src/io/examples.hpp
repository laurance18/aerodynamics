// examples.hpp: the example scenes built into the exe, for the Examples menu.
//
// They are the files in examples/, embedded at build time, so the standalone exe needs no
// examples folder. Each one is loaded like a scene file (see scene_io.hpp).
#pragma once

#include "io/scene_io.hpp"

#include <span>

namespace pfl {

struct ExampleScene {
    const char* file;        // file name in examples/, e.g. "cylinder.json"
    const char* title;       // menu label
    const char* description; // tooltip
};

// Every built-in example, in menu order.
std::span<const ExampleScene> exampleScenes();

SceneLoadResult loadExampleScene(const ExampleScene& example);

} // namespace pfl
