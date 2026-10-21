// platform_win32.hpp: the few Windows-specific services the app needs.
#pragma once

#include <string_view>

namespace pfl {

// Writes a line to stderr (visible in Debug's console) and to the debugger output window.
void logMessage(std::string_view message);

// Logs the message and shows it in a modal Windows MessageBox (works without a console).
void showFatalError(std::string_view title, std::string_view message);

} // namespace pfl
