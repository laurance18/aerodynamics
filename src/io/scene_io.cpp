#include "io/scene_io.hpp"

#include <nlohmann/json.hpp>

namespace pfl {

std::string sceneFileHeaderJson() {
    const nlohmann::json header = {{"format", "pfl-scene"}, {"version", 1}};
    return header.dump();
}

} // namespace pfl
