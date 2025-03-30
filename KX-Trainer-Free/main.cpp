#include "hack.h"
#include "hack_gui.h"
#include "kx_status.h"
#include "d3d_manager.h"
#include "status_ui.h"

#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <future>
#include <chrono>
#include <exception>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// Forward declarations
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global instance of the core hack logic
std::unique_ptr<Hack> g_hack;

// Wrapper for background initialization of the Hack object
std::unique_ptr<Hack> InitializeHackInBackground() {
    try {
        // Pass the thread-safe status message callback to the Hack constructor.
        // Assumes Hack constructor accepts std::function<void(const std::string&)>.
        return std::make_unique<Hack>(StatusUI::AddMessage);
    }
    catch (const std::exception& e) {
        StatusUI::AddMessage("ERROR: Exception during Hack initialization: " + std::string(e.what()));
        throw; // Re-throw for future::get()
    }
    catch (...) {
        StatusUI::AddMessage("ERROR: Unknown exception during Hack initialization.");
        throw; // Re-throw for future::get()
    }
}

// --- Application Entry Point ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // --- Preliminary Checks ---
    KXStatus statusCheck;
    if (!statusCheck.CheckStatus()) {
        MessageBox(NULL, _T("Status check failed! See logs or contact support. Exiting."), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    StatusUI::AddMessage("INFO: Status checks passed.");

    // --- Window Setup ---
    const TCHAR* CLASS_NAME = _T("KXTrainerHostWindowClass");
    const TCHAR* WINDOW_TITLE_LOADING = _T("KX Trainer - Loading...");
    const TCHAR* WINDOW_TITLE_ERROR = _T("KX Trainer - Error");
    const TCHAR* WINDOW_TITLE_HIDDEN = _T("KX Trainer (Hidden Host)");

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, CLASS_NAME, NULL };
    if (!::RegisterClassEx(&wc)) {
        MessageBox(NULL, _T("Failed to register window class."), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    StatusUI::AddMessage("INFO: Window class registered.");

    int initialWindowWidth = 450;
    int initialWindowHeight = 250;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - initialWindowWidth) / 2;
    int windowY = (screenHeight - initialWindowHeight) / 2;

    HWND hwnd = ::CreateWindowEx(0, CLASS_NAME, WINDOW_TITLE_LOADING, WS_POPUP | WS_VISIBLE,
        windowX, windowY, initialWindowWidth, initialWindowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, _T("Failed to create host window."), _T("Error"), MB_OK | MB_ICONERROR);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::SetForegroundWindow(hwnd); // Attempt to bring loading window to front
    ::BringWindowToTop(hwnd);
    StatusUI::AddMessage("INFO: Host window created.");

    // --- Initialize Graphics & ImGui ---
    if (!D3DManager::Initialize(hwnd)) {
        MessageBox(NULL, _T("Direct3D Initialization Failed. Check drivers and DirectX installation."), _T("Error"), MB_OK | MB_ICONERROR);
        D3DManager::Shutdown();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    StatusUI::AddMessage("INFO: DirectX 11 initialized.");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr; // Disable .ini saving

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(D3DManager::GetDevice(), D3DManager::GetDeviceContext());
    StatusUI::AddMessage("INFO: ImGui initialized.");

    // --- Start Background Initialization ---
    StatusUI::AddMessage("INFO: Starting background initialization...");
    auto future_hack = std::async(std::launch::async, InitializeHackInBackground);
    bool isLoading = true;
    bool initializationSuccess = false;
    std::string initializationErrorMsg = "";
    std::unique_ptr<HackGUI> gui = nullptr;
    bool gui_first_frame_rendered = false;

    // --- Main Loop ---
    bool done = false;
    while (!done) {
        // --- Process Window Messages ---
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        // --- Start ImGui Frame ---
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // --- Handle Application State ---
        if (isLoading) {
            if (future_hack.valid()) {
                auto status = future_hack.wait_for(std::chrono::milliseconds(0)); // Non-blocking check
                if (status == std::future_status::ready) {
                    try {
                        g_hack = future_hack.get(); // Will re-throw if exception occurred
                        initializationSuccess = (g_hack != nullptr);
                        if (initializationSuccess) {
                            StatusUI::AddMessage("INFO: Background initialization successful.");
                            ::ShowWindow(hwnd, SW_HIDE);
                            ::SetWindowText(hwnd, WINDOW_TITLE_HIDDEN);
                            gui = std::make_unique<HackGUI>(*g_hack); // Create GUI now
                            StatusUI::AddMessage("INFO: Main GUI created.");
                        }
                        else {
                            initializationErrorMsg = "Hack initialization returned null pointer.";
                            StatusUI::AddMessage("ERROR: " + initializationErrorMsg);
                            ::SetWindowText(hwnd, WINDOW_TITLE_ERROR);
                            if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW);
                        }
                    }
                    catch (const std::exception& e) {
                        initializationSuccess = false;
                        initializationErrorMsg = e.what();
                        StatusUI::AddMessage("ERROR: Caught exception: " + initializationErrorMsg);
                        ::SetWindowText(hwnd, WINDOW_TITLE_ERROR);
                        if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW);
                    }
                    catch (...) {
                        initializationSuccess = false;
                        initializationErrorMsg = "An unknown error occurred during initialization.";
                        StatusUI::AddMessage("ERROR: " + initializationErrorMsg);
                        ::SetWindowText(hwnd, WINDOW_TITLE_ERROR);
                        if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW);
                    }
                    isLoading = false;
                }
            }
            // Render status/loading/error window
            if (StatusUI::Render(initializationErrorMsg)) {
                done = true; // User requested exit from error screen
            }
        }
        else if (initializationSuccess && gui) {
            // Render main application GUI
            if (gui->renderUI()) { // renderUI returns true on exit request
                done = true;
            }
            // Bring the main GUI window to front once (mainly for viewports)
            if (!gui_first_frame_rendered && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
                ImGuiViewport* main_viewport = ImGui::GetMainViewport();
                HWND hwnd_to_focus = main_viewport ? (HWND)main_viewport->PlatformHandleRaw : hwnd;
                if (hwnd_to_focus && ::IsWindow(hwnd_to_focus)) {
                    ::BringWindowToTop(hwnd_to_focus);
                    ::SetForegroundWindow(hwnd_to_focus);
                }
                gui_first_frame_rendered = true;
            }
        }
        else {
            // Initialization failed, keep rendering the error window
            if (StatusUI::Render(initializationErrorMsg)) {
                done = true;
            }
        }

        // --- Rendering ---
        ImGui::Render();

        ID3D11RenderTargetView* rtv = D3DManager::GetMainRenderTargetView();
        ID3D11DeviceContext* ctx = D3DManager::GetDeviceContext();
        IDXGISwapChain* swapChain = D3DManager::GetSwapChain();

        if (rtv && ctx && swapChain) {
            const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            ctx->OMSetRenderTargets(1, &rtv, NULL);
            ctx->ClearRenderTargetView(rtv, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            HRESULT hr = swapChain->Present(1, 0); // Present with vsync

            // Handle device lost/reset
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                StatusUI::AddMessage("ERROR: Graphics device lost or reset. Exiting.");
                MessageBox(hwnd, _T("Graphics device lost. Application needs to close."), _T("Error"), MB_OK | MB_ICONERROR);
                done = true; // TODO: Implement robust device recovery if needed
            }
        }
        else if (!done) {
            StatusUI::AddMessage("WARN: Render target or device context not available. Skipping frame render.");
            Sleep(100); // Avoid spinning CPU
        }
    } // End main loop

    // --- Cleanup ---
    StatusUI::AddMessage("INFO: Shutting down...");
    g_hack.reset(); // Destroy Hack logic first
    gui.reset();    // Destroy GUI

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    D3DManager::Shutdown(); // Shutdown DirectX

    if (hwnd) ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    StatusUI::AddMessage("INFO: Shutdown complete.");
    // Note: Status messages might not be visible after this point if logging to file isn't implemented.

    return 0;
}

// --- Windows Message Procedure ---
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Allow ImGui to handle messages first
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        // Handle window resizing for DirectX backend
        if (D3DManager::GetDevice() != nullptr && wParam != SIZE_MINIMIZED) {
            UINT width = (UINT)LOWORD(lParam);
            UINT height = (UINT)HIWORD(lParam);
            if (width > 0 && height > 0) {
                D3DManager::HandleResize(width, height);
            }
        }
        return 0;

    case WM_SYSCOMMAND:
        // Prevent ALT menu activation
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0); // Signal main loop to exit
        return 0;

    case WM_DPICHANGED:
        // Optional: Handle DPI changes if required (update ImGui scaling, window size etc.)
        break;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}