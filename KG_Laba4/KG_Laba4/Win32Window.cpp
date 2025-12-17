#include "Win32Window.h"
#include "InputDevice.h"

static void SetUserData(HWND hWnd, void* ptr)
{
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
}

static Win32Window* GetUserData(HWND hWnd)
{
    return reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
}

bool Win32Window::Create(HINSTANCE hInstance,
    int nCmdShow,
    int clientWidth,
    int clientHeight,
    const wchar_t* className,
    const wchar_t* title)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Win32Window::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = className;

    if (!RegisterClassExW(&wc))
        return false;

    RECT rect{ 0,0, clientWidth, clientHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        className,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr, nullptr,
        hInstance,
        this
    );

    if (!m_hWnd)
        return false;

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
    return true;
}

LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        m_exitRequested = true;
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (m_input) m_input->OnKeyDown(wParam);
        if (wParam == VK_ESCAPE) {
            m_exitRequested = true;
            PostQuitMessage(0);
        }
        return 0;

    case WM_KEYUP:
        if (m_input) m_input->OnKeyUp(wParam);
        return 0;
    }

    return DefWindowProcW(m_hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* window = reinterpret_cast<Win32Window*>(cs->lpCreateParams);
        SetUserData(hWnd, window);
        window->m_hWnd = hWnd;
    }

    Win32Window* window = GetUserData(hWnd);
    if (window)
        return window->HandleMessage(msg, wParam, lParam);

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

