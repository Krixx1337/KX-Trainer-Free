#include "Application.h" // Include the new Application class header
#include <windows.h>
#include <stdexcept> // For exception handling if needed at entry point

// Note: Includes, global variables, and functions previously here
// have been moved into the Application class.


int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // Optional: Set high DPI awareness (requires <ShellScalingApi.h> and Shcore.lib)
    // ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    try {
        Application app(hInstance);

        if (app.Initialize()) {
            app.Run(); // Contains the main message loop and rendering
        }
        // Shutdown is handled by the Application destructor when 'app' goes out of scope
        // or can be called explicitly if preferred: app.Shutdown();

    } catch (const std::exception& e) {
        // Catch potential exceptions during Application construction or critical init failures
        std::string errorMsg = "Unhandled exception at application entry point: ";
        errorMsg += e.what();
        MessageBoxA(NULL, errorMsg.c_str(), "Critical Error", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        MessageBoxA(NULL, "An unknown unhandled exception occurred at application entry point.", "Critical Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
