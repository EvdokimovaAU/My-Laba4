#pragma once
#include <windows.h>
#include <unordered_map>

class InputDevice {
public:
    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);

    bool IsKeyDown(WPARAM key) const;

private:
    std::unordered_map<WPARAM, bool> m_keys;
};
