#pragma once
#ifndef APPLICATION_H
#define APPLICATION_H

#include <windows.h>
#include <string>
#include <memory>
#include <future>

// Forward declarations
class Hack;
class HackGUI;
class ILogger; // Forward declare logger interface
struct ImGuiIO; // Forward declare to avoid including imgui.h here if possible

class Application {
public:
    Application(HINSTANCE hInstance);
    ~Application();

    // Initialization steps
    bool Initialize();

    // Main loop execution
    void Run();

    // Cleanup
    void Shutdown(); // Consider making this implicit via destructor

    // Static WndProc to be registered with the window class
    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Application state
    enum class AppState {
        PreInit,
        InitializingStatusCheck,
        InitializingBackground,
        Running,
        Error,
        Exiting
    };

    // Initialization helpers
    bool InitializeWindow();
    bool InitializeGraphics();
    bool InitializeUI();
    bool PerformStatusCheck();
    void StartBackgroundInitialization();

    // Main loop helpers
    void ProcessMessages();
    void UpdateState();
    void RenderFrame();

    // Cleanup helper
    void Cleanup();

    // Window procedure instance method
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Member variables
    HINSTANCE m_hInstance;
    HWND m_hWnd = nullptr;
    std::unique_ptr<Hack> m_hack;
    std::unique_ptr<HackGUI> m_gui;
    std::future<std::unique_ptr<Hack>> m_future_hack;
    std::shared_ptr<ILogger> m_logger; // Logger instance

    AppState m_currentState = AppState::PreInit;
    std::wstring m_windowTitle; // Use wstring for Windows API
    std::string m_errorMessage;
    bool m_exitRequested = false;
    bool m_guiFirstFrameRendered = false;

    // Constants for window titles
    static const wchar_t* const WINDOW_TITLE_LOADING;
    static const wchar_t* const WINDOW_TITLE_ERROR;
    static const wchar_t* const WINDOW_TITLE_RUNNING; // Changed from HIDDEN
    static const wchar_t* const CLASS_NAME;
};

#endif // APPLICATION_H