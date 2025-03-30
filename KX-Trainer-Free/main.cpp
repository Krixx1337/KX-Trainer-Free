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
std::vector<std::string> g_statusMessages;
std::mutex g_statusMutex;
std::unique_ptr<Hack> g_hack;

// GUI Status Callback
void guiStatusCallback(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    g_statusMessages.push_back(message);
    const size_t MAX_MESSAGES = 100;
    if (g_statusMessages.size() > MAX_MESSAGES) {
        g_statusMessages.erase(g_statusMessages.begin());
    }
}

// Background Initialization Task
std::unique_ptr<Hack> InitializeHackInBackground() {
    try {
        return std::make_unique<Hack>(guiStatusCallback);
    }
    catch (...) {
        throw;
    }
}

// Minimalistic Status/Loading Window Renderer
void RenderStatusWindow(bool& should_exit, const std::string& error_msg = "") {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("StatusWindow", NULL, window_flags);

    ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
    float textHeight = ImGui::GetTextLineHeightWithSpacing();
    float buttonHeight = 0;
    if (!error_msg.empty()) {
        buttonHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    }
    float totalContentHeight = textHeight + buttonHeight;
    float posY = (contentRegionAvail.y - totalContentHeight) * 0.5f;
    if (posY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + posY);

    if (!error_msg.empty()) {
        const char* errorText = "Initialization Failed!";
        float textWidth = ImGui::CalcTextSize(errorText).x;
        float textPosX = (contentRegionAvail.x - textWidth) * 0.5f;
        if (textPosX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPosX);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), errorText);

        ImGui::TextWrapped("Error: %s", error_msg.c_str());
        ImGui::Separator();

        float buttonWidth = 120.0f;
        float buttonPosX = (contentRegionAvail.x - buttonWidth) * 0.5f;
        if (buttonPosX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonPosX);
        if (ImGui::Button("Exit", ImVec2(buttonWidth, 0))) {
            should_exit = true;
        }
    }
    else {
        const char* loadingText = "Initializing KX Trainer...";
        float textWidth = ImGui::CalcTextSize(loadingText).x;
        float textPosX = (contentRegionAvail.x - textWidth) * 0.5f;
        if (textPosX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPosX);
        ImGui::TextUnformatted(loadingText);
    }

    ImGui::End();
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    KXStatus status;
    if (!status.CheckStatus()) {
        MessageBox(NULL, _T("Status check failed! Exiting."), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create borderless host window for loading/error screen
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("KXTrainerHost"), NULL };
    ::RegisterClassEx(&wc);

    int windowWidth = 300;
    int windowHeight = 100;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    HWND hwnd = ::CreateWindowEx(0, wc.lpszClassName, _T("KX Trainer - Loading"), WS_POPUP | WS_VISIBLE,
        windowX, windowY, windowWidth, windowHeight,
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, _T("Failed to create host window."), _T("Error"), MB_OK | MB_ICONERROR);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Initialize Direct3D & ImGui
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D(); MessageBox(NULL, _T("Direct3D Initialization Failed."), _T("Error"), MB_OK | MB_ICONERROR);
        ::DestroyWindow(hwnd); ::UnregisterClass(wc.lpszClassName, wc.hInstance); return 1;
    }

    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = NULL; // Disable imgui.ini

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f; style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Start Hack initialization in background
    guiStatusCallback("INFO: Starting Hack initialization...");
    auto future_hack = std::async(std::launch::async, InitializeHackInBackground);
    bool isLoading = true;
    bool initializationSuccess = false;
    std::string initializationErrorMsg = "";
    std::unique_ptr<HackGUI> gui = nullptr;

    // Main loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg); ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();

        if (isLoading) {
            RenderStatusWindow(done, initializationErrorMsg);

            if (future_hack.valid()) {
                auto status = future_hack.wait_for(std::chrono::milliseconds(0));
                if (status == std::future_status::ready) {
                    try {
                        g_hack = future_hack.get();
                        if (!g_hack) throw std::runtime_error("Initialization returned null.");
                        initializationSuccess = true;
                        guiStatusCallback("INFO: Hack initialization successful.");
                        ::ShowWindow(hwnd, SW_HIDE);
                        SetWindowText(hwnd, _T("KX Trainer (Hidden Host)"));
                    }
                    catch (const std::exception& e) {
                        initializationSuccess = false; initializationErrorMsg = e.what();
                        guiStatusCallback("ERROR: Hack init failed: " + initializationErrorMsg);
                        SetWindowText(hwnd, _T("KX Trainer - Error"));
                    }
                    catch (...) {
                        initializationSuccess = false; initializationErrorMsg = "Unknown initialization error.";
                        guiStatusCallback("ERROR: Hack init failed (unknown).");
                        SetWindowText(hwnd, _T("KX Trainer - Error"));
                    }
                    isLoading = false;
                }
            }
        }
        else if (initializationSuccess) {
            if (!gui) gui = std::make_unique<HackGUI>(*g_hack);

            if (gui && gui->renderUI()) {
                done = true;
            }
        }
        else {
            RenderStatusWindow(done, initializationErrorMsg);
        }

        ImGui::Render();

        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        if (g_mainRenderTargetView) {
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        if (g_pSwapChain) {
            g_pSwapChain->Present(1, 0); // Present with VSync
        }
    }

    // Cleanup
    g_hack.reset(); gui.reset();
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D();
    if (hwnd) ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}

// --- Helper Functions ---

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    RECT rc; ::GetClientRect(hWnd, &rc);
    sd.BufferDesc.Width = (rc.right > 0) ? rc.right : 1;
    sd.BufferDesc.Height = (rc.bottom > 0) ? rc.bottom : 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = 0;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (FAILED(res)) {
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
        if (FAILED(res)) return false;
    }
    CreateRenderTarget();
    return true;
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (!g_pSwapChain) return;
    DXGI_SWAP_CHAIN_DESC desc;
    if (SUCCEEDED(g_pSwapChain->GetDesc(&desc)) && desc.BufferDesc.Width > 0 && desc.BufferDesc.Height > 0) {
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    }
    if (pBackBuffer && g_pd3dDevice) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    }
    else {
        g_mainRenderTargetView = nullptr;
    }
    if (pBackBuffer) pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            if (g_pSwapChain) g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}