#include "Application.h"
#include "hack.h"
#include "hack_gui.h"
#include "kx_status.h"
#include "d3d_manager.h"
#include "status_ui.h" // Still needed for StatusUI::Render and GetMessages/ClearMessages via logger
#include "gui_style.h"
#include "resource.h"
#include "ILogger.h"
#include "StatusUILogger.h"
#include <tchar.h>
#include <stdexcept>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const wchar_t* const Application::WINDOW_TITLE_LOADING = L"KX Trainer - Loading...";
const wchar_t* const Application::WINDOW_TITLE_ERROR = L"KX Trainer - Error";
const wchar_t* const Application::WINDOW_TITLE_RUNNING = L"KX Trainer"; // Changed from HIDDEN
const wchar_t* const Application::CLASS_NAME = L"KXTrainerHostWindowClass";


Application::Application(HINSTANCE hInstance)
    : m_hInstance(hInstance)
{
}

Application::~Application()
{
    Cleanup();
}


bool Application::Initialize()
{
    m_currentState = AppState::InitializingStatusCheck;

    if (!PerformStatusCheck()) {
        m_currentState = AppState::Error;
        // Need window/graphics/UI initialized to show the error message via StatusUI::Render
        if (!InitializeWindow()) return false; // Critical failure if window can't be made
        if (!InitializeGraphics()) return false; // Need graphics to show error UI
        if (!InitializeUI()) return false; // Need UI framework
        return true; // Return true because we initialized enough to show the error state
    }

    if (!InitializeWindow()) return false;
    m_logger = std::make_shared<StatusUILogger>();
    if (!m_logger) {
         MessageBox(NULL, L"Failed to create logger instance.", L"Critical Error", MB_OK | MB_ICONERROR);
         return false;
    }

    if (!InitializeGraphics()) return false;
    if (!InitializeUI()) return false;

    StartBackgroundInitialization();
    m_currentState = AppState::InitializingBackground;

    return true;
}

bool Application::PerformStatusCheck()
{
    KXStatus statusCheck;
    if (!statusCheck.CheckStatus()) {
        m_errorMessage = "Application status check failed. See log for details.";
        m_windowTitle = WINDOW_TITLE_ERROR;
        return false;
    }
    m_windowTitle = WINDOW_TITLE_LOADING;
    return true;
}


bool Application::InitializeWindow()
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Application::StaticWndProc, 0L, 0L, m_hInstance, NULL, NULL, NULL, NULL, CLASS_NAME, NULL };
    // TODO: Load icon properly using LoadIcon with IDI_ICON1 from resource.h
    // wc.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON1));
    // wc.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON1));

    if (!::RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Failed to register window class.", L"Critical Error", MB_OK | MB_ICONERROR);
        return false;
    }

    int initialWindowWidth = 450;
    int initialWindowHeight = 250; // Initial size for loading/error window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowX = (screenWidth - initialWindowWidth) / 2;
    int windowY = (screenHeight - initialWindowHeight) / 2;

    m_hWnd = ::CreateWindowEx(
        0,
        CLASS_NAME,                     // Window class
        m_windowTitle.c_str(),          // Window text
        WS_OVERLAPPEDWINDOW,

        windowX, windowY, initialWindowWidth, initialWindowHeight,

        NULL,
        NULL,
        m_hInstance,
        this        // Pass 'this' pointer to associate with window handle
    );

    if (!m_hWnd) {
        MessageBox(NULL, L"Failed to create host window.", L"Critical Error", MB_OK | MB_ICONERROR);
        ::UnregisterClass(CLASS_NAME, m_hInstance);
        return false;
    }

    ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    ::ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(m_hWnd);
    ::SetForegroundWindow(m_hWnd);
    ::BringWindowToTop(m_hWnd);

    return true;
}

bool Application::InitializeGraphics()
{
    if (!D3DManager::Initialize(m_hWnd)) {
        MessageBox(m_hWnd, L"Direct3D Initialization Failed. Check drivers and DirectX installation.", L"Critical Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

bool Application::InitializeUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    if (!GUIStyle::LoadAppFont(16.0f)) {
        if (m_logger) m_logger->Log(LogLevel::Warning, "Failed to load custom Bahnschrift font. Using default ImGui font.");
    }
    GUIStyle::ApplyCustomStyle();

    if (!ImGui_ImplWin32_Init(m_hWnd)) {
         MessageBox(m_hWnd, L"Failed to initialize ImGui Win32 backend.", L"Critical Error", MB_OK | MB_ICONERROR);
         return false;
    }
    if (!ImGui_ImplDX11_Init(D3DManager::GetDevice(), D3DManager::GetDeviceContext())) {
         MessageBox(m_hWnd, L"Failed to initialize ImGui DX11 backend.", L"Critical Error", MB_OK | MB_ICONERROR);
         return false;
    }

    return true;
}

// Background initialization task function
std::unique_ptr<Hack> InitializeHackTask(std::shared_ptr<ILogger> logger) {
     try {
        if (!logger) {
             // Cannot log this error easily without a logger!
             // Throwing an exception is reasonable here.
             throw std::runtime_error("Logger instance is null in InitializeHackTask");
        }

        // Stage 1: Create the Hack object
        auto hack_ptr = std::make_unique<Hack>(StatusUI::AddMessage); // Reverted: Pass StatusUI callback for now

        // Stage 2: Perform Hack initialization (process attach, scans)
        if (hack_ptr && hack_ptr->Initialize()) {
            logger->Log(LogLevel::Info, "Hack initialization task completed successfully.");
            return hack_ptr;
        } else {
            // Error messages should have been logged by Hack::Initialize
            logger->Log(LogLevel::Error, "Hack initialization task failed.");
            return nullptr;
        }
    }
    catch (const HackInitializationError& e) {
         if(logger) logger->Log(LogLevel::Critical, "HackInitializationError during background task: " + std::string(e.what()));
         return nullptr;
    }
    catch (const std::exception& e) {
        if(logger) logger->Log(LogLevel::Critical, "Exception during background initialization task: " + std::string(e.what()));
        return nullptr;
    }
    catch (...) {
        if(logger) logger->Log(LogLevel::Critical, "Unknown exception during background initialization task.");
        return nullptr;
    }
}

void Application::StartBackgroundInitialization()
{
    m_future_hack = std::async(std::launch::async, InitializeHackTask, m_logger);
}



void Application::Run()
{
    while (!m_exitRequested)
    {
        ProcessMessages();
        if (m_exitRequested) break;

        UpdateState();

        RenderFrame();
    }
}

void Application::ProcessMessages()
{
    MSG msg = {};
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            m_exitRequested = true;
            m_currentState = AppState::Exiting;
        }
    }
}

void Application::UpdateState()
{
    if (m_currentState == AppState::InitializingBackground) {
        if (m_future_hack.valid()) {
            auto status = m_future_hack.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready) {
                try {
                    m_hack = m_future_hack.get();

                    if (m_hack) {
                        m_gui = std::make_unique<HackGUI>(*m_hack); // Create GUI
                        m_currentState = AppState::Running;
                        ::ShowWindow(m_hWnd, SW_HIDE); // <<< Hide the host window here
                        m_windowTitle = WINDOW_TITLE_RUNNING;
                        ::SetWindowText(m_hWnd, m_windowTitle.c_str());
                    } else {
                        // Hack initialization failed. Error should be logged by InitializeHackTask.
                        m_currentState = AppState::Error;
                        if (m_errorMessage.empty()) {
                            m_errorMessage = "Core initialization failed. See log for details.";
                        }
                        m_windowTitle = WINDOW_TITLE_ERROR;
                        ::SetWindowText(m_hWnd, m_windowTitle.c_str());
                    }
                } catch (const std::exception& e) {
                    m_currentState = AppState::Error;
                    m_errorMessage = "Exception retrieving initialization result: " + std::string(e.what());
                    if(m_logger) m_logger->Log(LogLevel::Critical, m_errorMessage);
                    m_windowTitle = WINDOW_TITLE_ERROR;
                     ::SetWindowText(m_hWnd, m_windowTitle.c_str());
                } catch (...) {
                    m_currentState = AppState::Error;
                    m_errorMessage = "Unknown exception retrieving initialization result.";
                    if(m_logger) m_logger->Log(LogLevel::Critical, m_errorMessage);
                    m_windowTitle = WINDOW_TITLE_ERROR;
                    ::SetWindowText(m_hWnd, m_windowTitle.c_str());
                }
            }
        } else {
             // Should not happen if state is InitializingBackground
             m_currentState = AppState::Error;
             m_errorMessage = "Internal error: Invalid future in background init state.";
             if(m_logger) m_logger->Log(LogLevel::Critical, m_errorMessage);
             m_windowTitle = WINDOW_TITLE_ERROR;
             ::SetWindowText(m_hWnd, m_windowTitle.c_str());
        }
    }
}


void Application::RenderFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    switch (m_currentState)
    {
        case AppState::InitializingBackground:
        case AppState::InitializingStatusCheck:
            if (StatusUI::Render("")) {
                 m_exitRequested = true; // Handle potential exit request from status UI
            }
            break;

        case AppState::Running:
            if (m_gui) {
                if (m_gui->renderUI()) {
                    m_exitRequested = true;
                }
                if (!m_guiFirstFrameRendered) {
                    HWND hwnd_to_focus = m_hWnd;
                    ImGuiIO& io = ImGui::GetIO();
                    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
                        if (main_viewport && main_viewport->PlatformHandleRaw) {
                             hwnd_to_focus = (HWND)main_viewport->PlatformHandleRaw;
                        }
                    }
                    if (hwnd_to_focus && ::IsWindow(hwnd_to_focus)) {
                        ::BringWindowToTop(hwnd_to_focus);
                        ::SetForegroundWindow(hwnd_to_focus);
                    }
                    m_guiFirstFrameRendered = true;
                }
            } else {
                ImGui::Text("Error: GUI not initialized in Running state!");
            }
            break;

        case AppState::Error:
             if (StatusUI::Render(m_errorMessage)) {
                m_exitRequested = true;
            }
            break;

        case AppState::PreInit:
        case AppState::Exiting:
            break;
    }


    ImGui::Render();

    ID3D11RenderTargetView* rtv = D3DManager::GetMainRenderTargetView();
    ID3D11DeviceContext* ctx = D3DManager::GetDeviceContext();
    IDXGISwapChain* swapChain = D3DManager::GetSwapChain();

    if (rtv && ctx && swapChain) {
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        ctx->OMSetRenderTargets(1, &rtv, NULL);
        ctx->ClearRenderTargetView(rtv, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT hr = swapChain->Present(1, 0); // Present with vsync

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            if(m_logger) m_logger->Log(LogLevel::Critical, "Graphics device lost or reset. Exiting.");
            MessageBox(m_hWnd, L"Graphics device lost. Application needs to close.", L"Error", MB_OK | MB_ICONERROR);
            m_exitRequested = true;
            m_currentState = AppState::Exiting;
        }
    } else if (!m_exitRequested && m_currentState != AppState::Exiting) {
        if(m_logger) m_logger->Log(LogLevel::Warning, "Render target, context, or swap chain not available. Skipping frame render.");
        Sleep(100);
    }
}



void Application::Shutdown()
{
    Cleanup();
}

void Application::Cleanup()
{
    m_gui.reset();
    m_hack.reset();

    if (ImGui::GetCurrentContext()) {
        if (D3DManager::GetDeviceContext()) ImGui_ImplDX11_Shutdown();
        if (m_hWnd) ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    D3DManager::Shutdown();

    if (m_hWnd) {
        ::DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
    ::UnregisterClass(CLASS_NAME, m_hInstance);
}



LRESULT CALLBACK Application::StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Application* pApp = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pApp = reinterpret_cast<Application*>(pCreate->lpCreateParams);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp)); // Store 'this' pointer
        if (pApp) {
            pApp->m_hWnd = hWnd;
        }
    } else {
        pApp = reinterpret_cast<Application*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pApp) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
            return true;
        }
        return pApp->HandleMessage(hWnd, msg, wParam, lParam);
    } else {
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

LRESULT Application::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (D3DManager::GetDevice() != nullptr && wParam != SIZE_MINIMIZED) {
            UINT width = (UINT)LOWORD(lParam);
            UINT height = (UINT)HIWORD(lParam);
            if (width > 0 && height > 0) {
                D3DManager::HandleResize(width, height);
            }
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        m_exitRequested = true;
        m_currentState = AppState::Exiting;
        return 0;

    case WM_DPICHANGED:
        break;

    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}