#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <mmsystem.h>

#include <chrono>
#include <thread>
#include "Application.h"
#include "Core/Config.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * @brief Windows message procedure callback.
 *
 * Handles system messages such as window destruction and forwards relevant events to ImGui.
 *
 * @param hwnd Handle to the window.
 * @param uMsg The message code.
 * @param wParam Additional message-specific information.
 * @param lParam Additional message-specific information.
 * @return Result of the message processing.
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief Entry point for the Windows application.
 *
 * Initializes the window, the application logic, and enters the main message loop.
 *
 * @param hInstance Handle to the current instance of the application.
 * @param hPrevInstance Always NULL in Win32.
 * @param lpCmdLine Pointer to the command line string.
 * @param nCmdShow Specifies how the window is to be shown.
 * @return Exit code of the application.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SetProcessDPIAware();
    const wchar_t CLASS_NAME[] = L"SpotlightRendererWindowClass";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    RECT wr = {0, 0, Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Spotlight Renderer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    Application app;
    if (!app.Initialize(hwnd))
    {
        MessageBoxW(hwnd, L"Failed to initialize Application!", L"Error", MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    bool running = true;

    // Frame limiter: target 60 FPS (~16.67ms per frame)
    using clock = std::chrono::steady_clock;
    constexpr auto target_frame_time = std::chrono::microseconds(16667);
    constexpr auto spin_threshold = std::chrono::milliseconds(2);

    // Improve Windows timer resolution
    timeBeginPeriod(1);

    while (running)
    {
        auto frame_start = clock::now();

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (running)
        {
            app.BeginFrame();
            app.RenderUI();
            app.EndFrame();

            // Hybrid wait: sleep for bulk, spin for precision
            auto remaining = target_frame_time - (clock::now() - frame_start);
            if (remaining > spin_threshold)
            {
                std::this_thread::sleep_for(remaining - spin_threshold);
            }
            while (clock::now() - frame_start < target_frame_time)
            {
                // Spin-wait for remaining time
            }
        }
    }

    timeEndPeriod(1);

    return 0;
}