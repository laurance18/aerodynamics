// scene_io.hpp: save/load scenes as versioned JSON.
//
// The file holds everything needed to reproduce a picture: every element (flows and walls) with
// its parameters, and the plot settings (view, streamline density, arrows, overlays). Angles are
// stored in degrees so the files are easy to read and edit by hand:
//     {"format": "pfl-scene", "version": 1,
//      "elements": [{"type": "uniform", "speed": 1, "alpha_deg": 0}, ...],
//      "plot": {"x_min": -4, "x_max": 4, ...}}
// Loading validates every element; a file with an invalid one is rejected with a message naming it.
#pragma once

#include "core/scene.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace pfl {

struct SceneLoadResult {
    std::optional<Scene> scene;
    std::string error; // set when scene is empty
};

std::string sceneToJson(const Scene& scene);
SceneLoadResult sceneFromJson(std::string_view text);

// File wrappers. saveSceneFile returns false and sets `error` on failure.
bool saveSceneFile(const Scene& scene, const std::filesystem::path& path, std::string& error);
SceneLoadResult loadSceneFile(const std::filesystem::path& path);

} // namespace pfl
