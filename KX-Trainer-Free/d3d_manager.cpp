#include "d3d_manager.h"
#include <dxgi.h>
#include <cstdio> // For error logging during init failure

namespace D3DManager {

    // Internal state
    static ID3D11Device* g_pd3dDevice = nullptr;
    static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    static IDXGISwapChain* g_pSwapChain = nullptr;
    static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
    static HWND                     g_hWnd = nullptr;

    // Forward declarations
    static void CreateRenderTargetInternal();
    static void CleanupRenderTargetInternal();

    bool Initialize(HWND hWnd) {
        if (!hWnd) return false;
        g_hWnd = hWnd;

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2; // Use 2 for flip model

        RECT rc;
        ::GetClientRect(hWnd, &rc);
        // Ensure width/height are at least 1
        sd.BufferDesc.Width = (rc.right - rc.left > 0) ? (rc.right - rc.left) : 1;
        sd.BufferDesc.Height = (rc.bottom - rc.top > 0) ? (rc.bottom - rc.top) : 1;

        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Recommended modern swap effect
        sd.Flags = 0;

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // Enable if D3D debug layer is installed and needed
#endif

        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };

        HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            createDeviceFlags, featureLevelArray, ARRAYSIZE(featureLevelArray),
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
            &featureLevel, &g_pd3dDeviceContext);

        // Fallback to WARP driver if hardware fails
        if (FAILED(res)) {
            fprintf(stderr, "D3D11CreateDeviceAndSwapChain (Hardware) failed: 0x%lx\n", res);
            res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
                featureLevelArray, ARRAYSIZE(featureLevelArray), D3D11_SDK_VERSION,
                &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
            if (FAILED(res)) {
                fprintf(stderr, "D3D11CreateDeviceAndSwapChain (WARP) failed: 0x%lx\n", res);
                Shutdown();
                return false;
            }
            fprintf(stderr, "Using WARP (Software) D3D11 Driver.\n");
        }

        CreateRenderTargetInternal();
        return true;
    }

    void Shutdown() {
        CleanupRenderTargetInternal();
        if (g_pSwapChain) {
            g_pSwapChain->SetFullscreenState(FALSE, NULL); // Ensure windowed before release
            g_pSwapChain->Release();
            g_pSwapChain = nullptr;
        }
        if (g_pd3dDeviceContext) {
            g_pd3dDeviceContext->ClearState();
            g_pd3dDeviceContext->Flush();
            g_pd3dDeviceContext->Release();
            g_pd3dDeviceContext = nullptr;
        }
        if (g_pd3dDevice) {
            g_pd3dDevice->Release();
            g_pd3dDevice = nullptr;
        }
        g_hWnd = nullptr;
    }

    void HandleResize(UINT width, UINT height) {
        if (!g_pSwapChain || width == 0 || height == 0) {
            return; // Cannot resize if invalid
        }

        CleanupRenderTargetInternal(); // Release existing RTV

        HRESULT hr = g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) {
            fprintf(stderr, "Error resizing swap chain buffers: 0x%lx\n", hr);
            // Consider more robust error handling / reinitialization if required
            return;
        }

        CreateRenderTargetInternal(); // Re-create the render target view
    }

    static void CreateRenderTargetInternal() {
        if (!g_pSwapChain || !g_pd3dDevice) return;

        ID3D11Texture2D* pBackBuffer = nullptr;
        HRESULT hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (SUCCEEDED(hr)) {
            hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release(); // RTV holds its own reference
            if (FAILED(hr)) {
                fprintf(stderr, "Error creating render target view: 0x%lx\n", hr);
                g_mainRenderTargetView = nullptr;
            }
        }
        else {
            fprintf(stderr, "Error getting swap chain buffer: 0x%lx\n", hr);
        }
    }

    static void CleanupRenderTargetInternal() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
        // Ensure the context doesn't have the RTV bound anymore
        if (g_pd3dDeviceContext) {
            ID3D11RenderTargetView* nullRTV = nullptr;
            g_pd3dDeviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);
            g_pd3dDeviceContext->Flush(); // Ensure command execution
        }
    }

    // --- Getters ---
    ID3D11Device* GetDevice() { return g_pd3dDevice; }
    ID3D11DeviceContext* GetDeviceContext() { return g_pd3dDeviceContext; }
    IDXGISwapChain* GetSwapChain() { return g_pSwapChain; }
    ID3D11RenderTargetView* GetMainRenderTargetView() { return g_mainRenderTargetView; }

} // namespace D3DManager