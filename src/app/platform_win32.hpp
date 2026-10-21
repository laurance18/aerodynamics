// platform_win32.hpp: the few Windows-specific services the app needs.
#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

struct GLFWwindow;

namespace pfl {

// Writes a line to stderr (visible in Debug's console) and to the debugger output window.
void logMessage(std::string_view message);

// Logs the message and shows it in a modal Windows MessageBox (works without a console).
void showFatalError(std::string_view title, std::string_view message);

// Same message box for recoverable errors (e.g. a file that could not be read).
void showErrorMessage(GLFWwindow* owner, std::string_view title, std::string_view message);

// The first command-line argument as a path (e.g. a scene file dropped on the exe), if any.
std::optional<std::filesystem::path> commandLineFile();

// Native Open / Save As dialogs for scene files (*.json). Empty if the user cancels.
std::optional<std::filesystem::path> showOpenSceneDialog(GLFWwindow* owner);
std::optional<std::filesystem::path> showSaveSceneDialog(GLFWwindow* owner, const std::filesystem::path& suggested);

} // namespace pfl
