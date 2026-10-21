#include "app/platform_win32.hpp"

#include <windows.h>

#include <cstdio>
#include <string>

namespace pfl {
namespace {

std::wstring widen(std::string_view utf8) {
    if (utf8.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), length);
    return wide;
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

} // namespace pfl
