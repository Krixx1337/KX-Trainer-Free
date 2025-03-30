#include "hack.h"
#include "hack_gui.h"
#include "kx_status.h"

#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <future>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// Global DirectX Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Globals
std::vector<std::string>    g_statusMessages;
std::mutex                  g_statusMutex;
std::unique_ptr<Hack>       g_hack;
const size_t                MAX_STATUS_MESSAGES = 15; // Max messages in status window history

// GUI Status Callback (thread-safe)
void guiStatusCallback(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    g_statusMessages.push_back(message);
    while (g_statusMessages.size() > MAX_STATUS_MESSAGES) {
        g_statusMessages.erase(g_statusMessages.begin()); // Remove oldest
    }
}

// Background Initialization Task Wrapper
std::unique_ptr<Hack> InitializeHackInBackground() {
    // Exceptions are caught by std::async/future.get()
    return std::make_unique<Hack>(guiStatusCallback);
}

// Renders the initial status/loading/error window
void RenderStatusWindow(bool& should_exit, const std::string& error_msg = "") {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
    ImGui::Begin("StatusWindow", NULL, window_flags);

    ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
    ImGuiStyle& style = ImGui::GetStyle();

    if (!error_msg.empty()) {
        // Error Display
        const char* errorTitle = "Initialization Failed!";
        float titleWidth = ImGui::CalcTextSize(errorTitle).x;
        float titlePosX = (contentRegionAvail.x - titleWidth) * 0.5f;
        if (titlePosX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + titlePosX);
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), errorTitle);

        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushTextWrapPos(contentRegionAvail.x - style.WindowPadding.x);
        ImGui::Text("Error Details:");
        ImGui::TextWrapped("%s", error_msg.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Position exit button near bottom-center
        float buttonWidth = 120.0f;
        float buttonPosX = (contentRegionAvail.x - buttonWidth) * 0.5f;
        ImGui::SetCursorPosY(contentRegionAvail.y - ImGui::GetFrameHeightWithSpacing() - style.WindowPadding.y);
        if (buttonPosX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonPosX);
        if (ImGui::Button("Exit Application", ImVec2(buttonWidth, 0))) {
            should_exit = true;
        }
    }
    else {
        // Loading Display
        const char* loadingTitle = "Initializing KX Trainer...";
        float titleWidth = ImGui::CalcTextSize(loadingTitle).x;
        float titlePosX = (contentRegionAvail.x - titleWidth) * 0.5f;
        if (titlePosX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + titlePosX);
        ImGui::TextUnformatted(loadingTitle);
        ImGui::Separator();

        // Scrolling child window for status messages
        ImGuiWindowFlags child_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("StatusLog", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, child_flags);

        { // Scope for mutex lock
            std::lock_guard<std::mutex> lock(g_statusMutex);
            for (const auto& msg : g_statusMessages) {
                ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text); // Default color
                if (msg.rfind("ERROR:", 0) == 0)      color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                else if (msg.rfind("WARN:", 0) == 0)  color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
                else if (msg.rfind("INFO:", 0) == 0)  color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                ImGui::TextColored(color, "%s", msg.c_str());
            }
        } // Mutex released here

        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Perform preliminary checks if needed
    KXStatus statusCheck;
    if (!statusCheck.CheckStatus()) {
        MessageBox(NULL, _T("Status check failed! Exiting."), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Register the window class for the host window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("KXTrainerHost"), NULL };
    ::RegisterClassEx(&wc);

    // Create the initial (potentially hidden later) host window centered on screen
    int windowWidth = 450;
    int windowHeight = 250;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;
    HWND hwnd = ::CreateWindowEx(0, wc.lpszClassName, _T("KX Trainer - Loading"), WS_POPUP | WS_VISIBLE,
        windowX, windowY, windowWidth, windowHeight, NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, _T("Failed to create host window."), _T("Error"), MB_OK | MB_ICONERROR);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Attempt to bring the loading window to the front
    ::SetForegroundWindow(hwnd);

    // Initialize Direct3D device and swap chain
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        MessageBox(NULL, _T("Direct3D Initialization Failed."), _T("Error"), MB_OK | MB_ICONERROR);
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Initialize ImGui context and backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = NULL; // Disable INI file saving/loading

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Start Hack initialization asynchronously
    auto future_hack = std::async(std::launch::async, InitializeHackInBackground);
    bool isLoading = true;
    bool initializationSuccess = false;
    std::string initializationErrorMsg = "";
    std::unique_ptr<HackGUI> gui = nullptr;
    bool gui_first_frame_rendered = false;

    // Main application loop
    bool done = false;
    while (!done) {
        // Process Windows messages
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Check background initialization status
        if (isLoading) {
            if (future_hack.valid()) {
                // Poll the future without blocking
                auto status = future_hack.wait_for(std::chrono::milliseconds(0));
                if (status == std::future_status::ready) {
                    try {
                        g_hack = future_hack.get(); // Retrieve result or exception
                        initializationSuccess = true;
                        ::ShowWindow(hwnd, SW_HIDE); // Hide host window on success
                        SetWindowText(hwnd, _T("KX Trainer (Hidden Host)"));
                    }
                    catch (const std::exception& e) {
                        initializationSuccess = false;
                        initializationErrorMsg = e.what();
                        SetWindowText(hwnd, _T("KX Trainer - Error"));
                        // Ensure error window is visible
                        if (!::IsWindowVisible(hwnd)) {
                            ::ShowWindow(hwnd, SW_SHOW);
                        }
                    }
                    isLoading = false; // Initialization attempt finished
                }
            }
            // Render status/loading/error window during initialization phase
            RenderStatusWindow(done, initializationErrorMsg);
        }
        else if (initializationSuccess) {
            // Initialization succeeded, render main GUI
            if (!gui) {
                gui = std::make_unique<HackGUI>(*g_hack);
            }
            if (gui && gui->renderUI()) { // renderUI returns true if user requests exit
                done = true;
            }
        }
        else {
            // Initialization failed, keep rendering the error window
            RenderStatusWindow(done, initializationErrorMsg);
        }

        // Render ImGui draw data
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f }; // Background doesn't matter much if hidden
        if (g_mainRenderTargetView) {
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        // Update and Render additional Platform Windows (for viewports)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Bring the main GUI window to front once after successful init
        if (initializationSuccess && gui && !gui_first_frame_rendered) {
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            // Prefer the actual ImGui viewport window handle if available
            HWND main_gui_hwnd = main_viewport ? (HWND)main_viewport->PlatformHandleRaw : NULL;
            HWND hwnd_to_focus = (main_gui_hwnd && main_gui_hwnd != hwnd) ? main_gui_hwnd : hwnd;

            BringWindowToTop(hwnd_to_focus);
            SetForegroundWindow(hwnd_to_focus);
            gui_first_frame_rendered = true;
        }

        // Present the frame
        if (g_pSwapChain) {
            g_pSwapChain->Present(1, 0); // Present with vsync
        }
    }

    // Cleanup
    g_hack.reset();
    gui.reset();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    if (hwnd) ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// --- DirectX Helper Functions ---

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    RECT rc; ::GetClientRect(hWnd, &rc);
    sd.BufferDesc.Width = (rc.right > 0) ? rc.right : 1; // Ensure non-zero size
    sd.BufferDesc.Height = (rc.bottom > 0) ? rc.bottom : 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Recommended swap effect
    sd.Flags = 0;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (FAILED(res)) { // Fallback to WARP driver if hardware fails
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
        if (FAILED(res)) return false;
    }

    CreateRenderTarget();
    return true;
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (!g_pSwapChain) return;

    // Ensure swap chain dimensions are valid before getting buffer
    DXGI_SWAP_CHAIN_DESC desc;
    if (SUCCEEDED(g_pSwapChain->GetDesc(&desc)) && desc.BufferDesc.Width > 0 && desc.BufferDesc.Height > 0) {
        if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)))) {
            if (g_pd3dDevice) {
                g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            }
            pBackBuffer->Release(); // Release our reference
        }
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->SetFullscreenState(FALSE, NULL); // Ensure windowed mode before release
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->ClearState();
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

// Win32 message handler forwarding to ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pSwapChain != nullptr && wParam != SIZE_MINIMIZED) {
            UINT width = (UINT)LOWORD(lParam);
            UINT height = (UINT)HIWORD(lParam);
            if (width > 0 && height > 0) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        // Disable ALT menu for the host window
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}