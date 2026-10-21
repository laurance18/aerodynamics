// main.cpp: process entry point. Sets up debug-heap leak reporting and runs the application.
#include "app/app.hpp"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

// Ask hybrid-graphics laptops (NVIDIA Optimus / AMD PowerXpress) to use the discrete GPU.
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main() {
#ifdef _DEBUG
    // Report any heap allocation still alive at process exit, to the debugger output window
    // and to stderr (so leaks are visible in the console too).
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif
    return pfl::runApp();
}
