#pragma once

#include <windows.h>

#include "Win32Window.h"
#include "InputDevice.h"
#include "GameTimer.h"
#include "D3D12Context.h"

class Application {
public:
    explicit Application(HINSTANCE hInstance);

    bool Initialize(int nCmdShow);
    int Run();

private:
    void Update(const GameTimer& timer);
    void Draw(const GameTimer& timer);

private:
    HINSTANCE m_hInstance = nullptr;

    Win32Window m_window;
    InputDevice m_input;
    GameTimer   m_timer;

    D3D12Context m_dx12;
};
