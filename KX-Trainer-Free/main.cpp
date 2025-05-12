#include "hack.h"
#include "hack_gui.h"
#include "kx_status.h"
#include "d3d_manager.h"
#include "status_ui.h"
#include "gui_style.h"

#include <windows.h>
#include <tchar.h>
#include <string>
#include <memory>
#include <future>
#include <chrono>
#include <exception>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<Hack> g_hack;

std::unique_ptr<Hack> InitializeHackInBackground() {
    try {
        // Stage 1: Create the Hack object (minimal constructor)
        auto hack_ptr = std::make_unique<Hack>(StatusUI::AddMessage);

        // Stage 2: Perform the actual initialization (process attach, scans)
        if (hack_ptr && hack_ptr->Initialize()) {
            return hack_ptr;
        } else {
            // Error messages should have been logged by Hack::Initialize via the callback
            return nullptr;
        }
    }
    catch (const std::exception& e) {
        // Catches exceptions during Hack *constructor* or other unexpected issues
        StatusUI::AddMessage("ERROR: Exception during background initialization task: " + std::string(e.what()));
        return nullptr;
    }
    catch (...) {
        StatusUI::AddMessage("ERROR: Unknown exception during background initialization task.");
        return nullptr;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    bool statusCheckOk = false;
    std::string statusCheckErrorMsg = "";

    // Preliminary Checks
    {
        KXStatus statusCheck;
        if (!statusCheck.CheckStatus()) {
            // Status check failed. KXStatus already logged the details via StatusUI.
            statusCheckOk = false;
            statusCheckErrorMsg = "Application status check failed. See log above for details.";
            // Specific error (outdated, disabled, API fail) is logged by KXStatus
        }
        else {
            statusCheckOk = true;
            // Success is implicit if no error was logged by KXStatus
        }
    }

    // Window Setup
    const TCHAR* CLASS_NAME = _T("KXTrainerHostWindowClass");
    const TCHAR* WINDOW_TITLE_LOADING = _T("KX Trainer - Loading...");
    const TCHAR* WINDOW_TITLE_ERROR = _T("KX Trainer - Error");
    const TCHAR* WINDOW_TITLE_HIDDEN = _T("KX Trainer (Hidden Host)");

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, CLASS_NAME, NULL };
    if (!::RegisterClassEx(&wc)) {
        // Critical failure, MessageBox is appropriate here as StatusUI isn't ready
        MessageBox(NULL, _T("Failed to register window class."), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    int initialWindowWidth = 450;
    int initialWindowHeight = 250;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - initialWindowWidth) / 2;
    int windowY = (screenHeight - initialWindowHeight) / 2;

    // Set initial title based on status check result
    const TCHAR* initialTitle = statusCheckOk ? WINDOW_TITLE_LOADING : WINDOW_TITLE_ERROR;
    HWND hwnd = ::CreateWindowEx(0, CLASS_NAME, initialTitle, WS_POPUP | WS_VISIBLE,
        windowX, windowY, initialWindowWidth, initialWindowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, _T("Failed to create host window."), _T("Error"), MB_OK | MB_ICONERROR);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::SetForegroundWindow(hwnd); // Attempt to bring loading window to front
    ::BringWindowToTop(hwnd);

    // Initialize Graphics & ImGui (Needed to display StatusUI)
    if (!D3DManager::Initialize(hwnd)) {
        // Critical failure, MessageBox appropriate
        MessageBox(NULL, _T("Direct3D Initialization Failed. Check drivers and DirectX installation."), _T("Error"), MB_OK | MB_ICONERROR);
        D3DManager::Shutdown();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr; // Disable .ini saving

    GUIStyle::LoadAppFont(16.0f);
    GUIStyle::ApplyCustomStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(D3DManager::GetDevice(), D3DManager::GetDeviceContext());

    // Initialize Application State
    bool isLoading = false; // Will be true only if status check passed AND we start background init
    bool initializationSuccess = statusCheckOk; // Start with the status check result
    std::string initializationErrorMsg = statusCheckErrorMsg; // Use the message from status check if it failed
    std::unique_ptr<HackGUI> gui = nullptr;
    std::future<std::unique_ptr<Hack>> future_hack;

    // Start Background Initialization ONLY IF status check passed
    if (initializationSuccess) {
        future_hack = std::async(std::launch::async, InitializeHackInBackground);
        isLoading = true; // Now we are actually loading the Hack object
    }
    else {
        // If status check failed, ensure the error window title is set (already done by CreateWindowEx)
        if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW); // Make sure window is visible
    }

    bool gui_first_frame_rendered = false;

    // Main Loop
    bool done = false;
    while (!done) {
        // Process Window Messages
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break; // Exit loop immediately if WM_QUIT received

        // Start ImGui Frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Handle Application State
        if (isLoading) { // Only true if status check passed and background task is running
            if (future_hack.valid()) {
                auto status = future_hack.wait_for(std::chrono::milliseconds(0)); // Non-blocking check
                if (status == std::future_status::ready) {
                    try {
                        // Get the result from the background task.
                        // This will be nullptr if InitializeHackInBackground failed.
                        g_hack = future_hack.get();
                        initializationSuccess = (g_hack != nullptr); // Check if we got a valid pointer

                        if (initializationSuccess) {
                            // Background task succeeded
                            gui = std::make_unique<HackGUI>(*g_hack); // Create GUI now
                            ::ShowWindow(hwnd, SW_HIDE);
                            ::SetWindowText(hwnd, WINDOW_TITLE_HIDDEN);
                        }
                        else {
                            // Hack::Initialize() or InitializeHackInBackground already logged the specific error.
                            // Set a generic message here if needed, but rely on logs.
                            if (initializationErrorMsg.empty()) { // Only set if status check didn't already fail
                                initializationErrorMsg = "Hack initialization failed. Ensure Guild Wars 2 is running. If it is, try running both the game and the trainer as Administrator. See log for more details.";
                            }
                            ::SetWindowText(hwnd, WINDOW_TITLE_ERROR);
                            if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW);
                        }
                    }
                    catch (const std::exception& e) {
                        // Catch potential exceptions from future::get() itself
                        initializationSuccess = false;
                        initializationErrorMsg = "Exception retrieving initialization result: " + std::string(e.what());
                        StatusUI::AddMessage("ERROR: " + initializationErrorMsg);
                        ::SetWindowText(hwnd, WINDOW_TITLE_ERROR);
                        if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW);
                    }
                    catch (...) {
                        // Catch potential exceptions from future::get() itself
                        initializationSuccess = false;
                        initializationErrorMsg = "An unknown error occurred during initialization.";
                        StatusUI::AddMessage("ERROR: " + initializationErrorMsg); // Log this specific error
                        ::SetWindowText(hwnd, WINDOW_TITLE_ERROR);
                        if (!::IsWindowVisible(hwnd)) ::ShowWindow(hwnd, SW_SHOW);
                    }
                    isLoading = false; // Background task attempt finished
                }
            }
            // Render status/loading window (shows "Initializing..." title if loading, empty if waiting)
            if (StatusUI::Render(isLoading ? "" : "Waiting for initialization task...")) { // Pass empty error message while loading Hack
                done = true;
            }
        }
        else if (initializationSuccess && gui) { // Status check OK, Hack init OK, GUI exists
            // Render main application GUI
            if (gui->renderUI()) { // Assuming renderUI returns true on exit request
                done = true;
            }
            // Bring the main GUI window to front once (if using viewports mainly)
            if (!gui_first_frame_rendered && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
                ImGuiViewport* main_viewport = ImGui::GetMainViewport();
                HWND hwnd_to_focus = main_viewport ? (HWND)main_viewport->PlatformHandleRaw : hwnd;
                if (hwnd_to_focus && ::IsWindow(hwnd_to_focus)) { // Check if handle is valid
                    ::BringWindowToTop(hwnd_to_focus);
                    ::SetForegroundWindow(hwnd_to_focus);
                }
                gui_first_frame_rendered = true;
            }
        }
        else { // Either status check failed OR Hack init failed
            // Render the error window using the message set earlier
            if (StatusUI::Render(initializationErrorMsg)) { // Pass the specific error message
                done = true;
            }
        }

        // Rendering
        ImGui::Render(); // End frame and prepare draw data

        ID3D11RenderTargetView* rtv = D3DManager::GetMainRenderTargetView();
        ID3D11DeviceContext* ctx = D3DManager::GetDeviceContext();
        IDXGISwapChain* swapChain = D3DManager::GetSwapChain();

        if (rtv && ctx && swapChain) {
            const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            ctx->OMSetRenderTargets(1, &rtv, NULL); // Bind the render target
            ctx->ClearRenderTargetView(rtv, clear_color); // Clear it
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Render ImGui data

            // Update and Render additional Platform Windows (for viewports)
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            // Present the frame
            HRESULT hr = swapChain->Present(1, 0); // Present with vsync (1)

            // Handle device lost/reset - Keep this essential error log
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                StatusUI::AddMessage("ERROR: Graphics device lost or reset. Exiting.");
                MessageBox(hwnd, _T("Graphics device lost. Application needs to close."), _T("Error"), MB_OK | MB_ICONERROR);
                done = true; // TODO: Implement robust device recovery if needed
            }
        }
        else if (!done) {
            // Keep this warning as it indicates a rendering problem
            StatusUI::AddMessage("WARN: Render target or device context not available. Skipping frame render.");
            Sleep(100); // Avoid spinning CPU if stuck in this state
        }
    }

    // Cleanup
    g_hack.reset(); // Ensure Hack object is destroyed before subsystems
    gui.reset();    // Destroy GUI object

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    D3DManager::Shutdown(); // Shutdown DirectX

    if (hwnd) ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Allow ImGui to handle messages first
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true; // Message was handled by ImGui

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
        // Prevent ALT menu activation (often unwanted in game overlays/tools)
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0; // Eat the message
        break; // Let default handler process other sys commands

    case WM_DESTROY:
        ::PostQuitMessage(0); // Signal application to exit
        return 0;

    case WM_DPICHANGED:
        // Optional: Handle DPI changes if required (update ImGui font scaling, window size etc.)
        break;
    }

    // For messages not handled, pass to default procedure
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
