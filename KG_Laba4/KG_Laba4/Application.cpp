#include "Application.h"
#include <windows.h>

Application::Application(HINSTANCE hInstance)
    : m_hInstance(hInstance)
{
}

bool Application::Initialize(int nCmdShow)
{
    m_window.SetInputDevice(&m_input);

    if (!m_window.Create(
        m_hInstance,
        nCmdShow,
        800,
        600,
        L"MyWindowClass",
        L"DX12 Part Two"))
    {
        return false;
    }

    if (!m_dx12.Initialize(m_window.GetHWND(), 800, 600))
    {
        MessageBoxW(nullptr, L"Failed to initialize DirectX 12", L"DX12 Error", MB_OK);
        return false;
    }

    m_timer.Reset();
    return true;
}

int Application::Run()
{
    MSG msg{};
    while (!m_window.IsExitRequested())
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        m_timer.Tick();
        Update(m_timer);
        Draw(m_timer);
    }

    m_dx12.Shutdown();
    return (int)msg.wParam;
}

void Application::Update(const GameTimer& /*timer*/)
{
    if (m_input.IsKeyDown(VK_ESCAPE))
        PostQuitMessage(0);
}

void Application::Draw(const GameTimer& timer)
{
    // ћожно сделать цвет фиксированный:
    // m_dx12.Render(0.1f, 0.2f, 0.7f, 1.0f);

    // »ли слегка мен€ющийс€ по времени, чтобы точно видеть, что кадры обновл€ютс€:
    float t = timer.TotalTime();
    float r = 0.5f + 0.5f * sinf(t);
    float g = 0.2f;
    float b = 0.6f;

    m_dx12.Render(r, g, b, 1.0f);
}

