// app.hpp: application lifetime: window + GL context, ImGui, main loop, orderly shutdown.
#pragma once

namespace pfl {

// Runs the application until the window closes. Returns the process exit code.
int runApp();

} // namespace pfl
