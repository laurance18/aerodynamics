#include "render/shader_sources.hpp"

#include "embedded_resources.hpp"

#include <fstream>
#include <sstream>

namespace pfl {

std::optional<std::string> loadShaderSource(std::string_view name) {
#ifdef PFL_SHADER_DIR
    std::ifstream file(std::string(PFL_SHADER_DIR) + "/" + std::string(name), std::ios::binary);
    if (file) {
        std::ostringstream text;
        text << file.rdbuf();
        return text.str();
    }
#endif
    if (auto embedded = embedded::findText("shaders/" + std::string(name))) return std::string(*embedded);
    return std::nullopt;
}

bool shaderHotReloadAvailable() {
#ifdef PFL_SHADER_DIR
    return true;
#else
    return false;
#endif
}

} // namespace pfl
