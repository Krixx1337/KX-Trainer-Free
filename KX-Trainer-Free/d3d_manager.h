#ifndef D3D_MANAGER_H
#define D3D_MANAGER_H

#include <d3d11.h>
#include <windows.h>

namespace D3DManager {

    // Initializes the D3D device, context, and swap chain for the given window.
    bool Initialize(HWND hWnd);

    // Cleans up all D3D resources.
    void Shutdown();

    // Handles window resizing for the swap chain and render target.
    void HandleResize(UINT width, UINT height);

    // Accessors for core D3D objects.
    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetDeviceContext();
    IDXGISwapChain* GetSwapChain();
    ID3D11RenderTargetView* GetMainRenderTargetView();

} // namespace D3DManager

#endif // D3D_MANAGER_H