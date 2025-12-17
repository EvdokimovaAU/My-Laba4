#pragma once
#include <windows.h>

class InputDevice;

class Win32Window {
public:
    bool Create(HINSTANCE hInstance,
        int nCmdShow,
        int clientWidth,
        int clientHeight,
        const wchar_t* className,
        const wchar_t* title);

    void SetInputDevice(InputDevice* input) { m_input = input; }

    HWND GetHWND() const { return m_hWnd; }
    bool IsExitRequested() const { return m_exitRequested; }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hWnd = nullptr;
    bool m_exitRequested = false;
    InputDevice* m_input = nullptr;
};
