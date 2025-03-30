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

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// Global DirectX Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr; // For the hidden host window swapchain

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global status (Optional - depends on HackGUI)
std::vector<std::string> g_statusMessages;
std::mutex g_statusMutex;
std::unique_ptr<Hack> g_hack;

// GUI Status Callback (Optional - depends on HackGUI)
void guiStatusCallback(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_statusMutex);
    g_statusMessages.push_back(message);
    const size_t MAX_MESSAGES = 100;
    if (g_statusMessages.size() > MAX_MESSAGES) {
        g_statusMessages.erase(g_statusMessages.begin(), g_statusMessages.begin() + (g_statusMessages.size() - MAX_MESSAGES));
    }
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initial checks (Optional)
    KXStatus status;
    if (!status.CheckStatus()) {
        MessageBox(NULL, _T("Status check failed! Exiting."), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create a hidden host window for initialization purposes
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("KXTrainerHiddenHost"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindowEx(0, wc.lpszClassName, _T("KX Trainer Hidden Host"), WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 50, 50,
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, _T("Failed to create host window."), _T("Error"), MB_OK | MB_ICONERROR);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Temporarily show window for initialization routines that require a visible window handle
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Initialize the core application logic (e.g., Hack)
    try {
        g_hack = std::make_unique<Hack>(guiStatusCallback);
    }
    catch (const std::runtime_error& e) {
        std::string errorMsg = "Initialization Error: "; errorMsg += e.what();
        MessageBoxA(NULL, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
        ::DestroyWindow(hwnd); ::UnregisterClass(wc.lpszClassName, wc.hInstance); return 1;
    }
    catch (...) {
        MessageBox(NULL, _T("Unknown initialization error."), _T("Error"), MB_OK | MB_ICONERROR);
        ::DestroyWindow(hwnd); ::UnregisterClass(wc.lpszClassName, wc.hInstance); return 1;
    }

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        MessageBox(NULL, _T("Failed to create Direct3D device."), _T("Error"), MB_OK | MB_ICONERROR);
        ::DestroyWindow(hwnd); ::UnregisterClass(wc.lpszClassName, wc.hInstance); return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Tweak style for viewports
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Hide the host window now that initialization is complete
    ::ShowWindow(hwnd, SW_HIDE);

    // Create the main GUI logic handler
    HackGUI gui(*g_hack);

    // Main loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render application GUI. ImGui::Begin/End calls inside gui.renderUI()
        // will create platform windows due to ViewportsEnable and hidden host.
        gui.renderUI();

        // Example of closing condition (adapt as needed, e.g., based on a GUI button)
        // if (gui.shouldExit()) { // Assuming HackGUI has a method like this
        //    done = true;
        // }

        // Rendering
        ImGui::Render();

        // --- Platform Window Rendering ---
        // RenderPlatformWindowsDefault is crucial for drawing the ImGui windows
        // when the main host window is hidden.
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Present the hidden main window's swap chain.
        // Required for backend processing even if not visible.
        g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    if (hwnd) {
        ::DestroyWindow(hwnd);
    }
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Preferred swap effect
    sd.Flags = 0;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // Uncomment for debug layer
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (FAILED(res)) {
        // Fallback to WARP driver if hardware fails? (Optional)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
        if (FAILED(res)) {
            return false;
        }
    }

    CreateRenderTarget(); // Create RTV for the swap chain (even if hidden)
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (g_pSwapChain) {
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    }
    if (pBackBuffer && g_pd3dDevice) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    }
    if (pBackBuffer) {
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        // WM_SIZE handling for the hidden window is generally not needed
        // case WM_SIZE:
        //     if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) { }
        //     return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}