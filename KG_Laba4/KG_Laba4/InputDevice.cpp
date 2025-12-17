#include "InputDevice.h"

void InputDevice::OnKeyDown(WPARAM key)
{
    m_keys[key] = true;
}

void InputDevice::OnKeyUp(WPARAM key)
{
    m_keys[key] = false;
}

bool InputDevice::IsKeyDown(WPARAM key) const
{
    auto it = m_keys.find(key);
    return it != m_keys.end() && it->second;
}
