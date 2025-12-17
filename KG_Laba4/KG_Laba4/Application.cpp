#include "Application.h"
#include <windows.h>

Application::Application(HINSTANCE hInstance, int nCmdShow)
    : m_hInstance(hInstance), m_nCmdShow(nCmdShow)
{
}

bool Application::Initialize()
{
    if (!m_window.Create(
        m_hInstance,
        m_nCmdShow,
        800,
        600,
        L"DX12WindowClass",
        L"KG_Laba4 - DX12 Final"))
    {
        MessageBoxW(nullptr, L"Window Create FAILED", L"Error", MB_OK);
        return false;
    }

    if (!m_dx12.Initialize(m_window.GetHWND(), 800, 600))
    {
        MessageBoxW(nullptr, L"DX12 Initialize FAILED (see Output window)", L"Error", MB_OK);
        return false;
    }

    m_timer.Reset();
    return true;
}

int Application::Run()
{
    MSG msg{};
    while (true)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                return (int)msg.wParam;

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // ESC закрывает без InputDevice
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            PostQuitMessage(0);
            continue;
        }

        m_timer.Tick();

        //m_dx12.SetRotation(m_timer.TotalTime());
        m_dx12.Render(0.08f, 0.12f, 0.20f, 1.0f);
    }
}
