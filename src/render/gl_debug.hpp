// gl_debug.hpp: routes driver messages (KHR_debug) to stderr in debug GL contexts.
#pragma once

namespace pfl {

// Installs the callback if the context is a debug context and KHR_debug / GL 4.3 is available.
// Returns true if installed.
bool installGlDebugOutput();

} // namespace pfl
