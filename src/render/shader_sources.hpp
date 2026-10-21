// shader_sources.hpp: look up GLSL source text by file name (e.g. "particles.vert").
//
// Release: always the copy embedded at build time. Debug (PFL_SHADERS_FROM_DISK=ON): the file
// in <source>/shaders/ is preferred so edits can be hot-reloaded; the embedded copy is the fallback.
#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pfl {

std::optional<std::string> loadShaderSource(std::string_view name);

// True when shaders are read from the source tree, i.e. reloading can pick up edits.
bool shaderHotReloadAvailable();

} // namespace pfl
