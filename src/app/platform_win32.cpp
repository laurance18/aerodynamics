#include "app/platform_win32.hpp"

#include <windows.h>

#include <commdlg.h>
#include <shellapi.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <array>
#include <cstdio>
#include <string>

namespace pfl {
namespace {

constexpr wchar_t k_sceneFilter[] = L"Flow scenes (*.json)\0*.json\0All files (*.*)\0*.*\0";

std::wstring widen(std::string_view utf8) {
    if (utf8.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), length);
    return wide;
}

HWND ownerWindow(GLFWwindow* window) { return window != nullptr ? glfwGetWin32Window(window) : nullptr; }

// Runs GetOpenFileNameW / GetSaveFileNameW with the scene filter. `initial` pre-fills the name.
std::optional<std::filesystem::path> fileDialog(GLFWwindow* owner, bool save, const std::filesystem::path& initial) {
    std::array<wchar_t, 4096> buffer{};
    initial.native().copy(buffer.data(), buffer.size() - 1);

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = ownerWindow(owner);
    ofn.lpstrFilter = k_sceneFilter;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                (save ? OFN_OVERWRITEPROMPT : static_cast<DWORD>(OFN_FILEMUSTEXIST));
    const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (ok == FALSE) return std::nullopt;
    return std::filesystem::path(buffer.data());
}

} // namespace

void logMessage(std::string_view message) {
    std::string line(message);
    line += '\n';
    std::fputs(line.c_str(), stderr);
    std::fflush(stderr);
    OutputDebugStringW(widen(line).c_str());
}

void showFatalError(std::string_view title, std::string_view message) {
    logMessage(std::string(title) + ": " + std::string(message));
    MessageBoxW(nullptr, widen(message).c_str(), widen(title).c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

void showErrorMessage(GLFWwindow* owner, std::string_view title, std::string_view message) {
    logMessage(std::string(title) + ": " + std::string(message));
    MessageBoxW(ownerWindow(owner), widen(message).c_str(), widen(title).c_str(), MB_OK | MB_ICONWARNING);
}

std::optional<std::filesystem::path> commandLineFile() {
    // CommandLineToArgvW gives Unicode arguments, unlike main()'s argv.
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (args == nullptr) return std::nullopt;
    std::optional<std::filesystem::path> path;
    if (count > 1) path = std::filesystem::path(args[1]);
    LocalFree(args);
    return path;
}

std::optional<std::filesystem::path> showOpenSceneDialog(GLFWwindow* owner) { return fileDialog(owner, false, {}); }

std::optional<std::filesystem::path> showSaveSceneDialog(GLFWwindow* owner, const std::filesystem::path& suggested) {
    return fileDialog(owner, true, suggested);
}

} // namespace pfl
