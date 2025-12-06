#pragma once

#include <windows.h>
#include <vector>

class InputDevice {
public:
    InputDevice(HWND hWnd);
    ~InputDevice();

    bool Initialize();
    void Update();
    void ProcessRawInput(LPARAM lParam);

    bool IsKeyDown(int vKey) const;
    int GetMouseDeltaX() const { return m_mouseDeltaX; }
    int GetMouseDeltaY() const { return m_mouseDeltaY; }

private:
    HWND m_hWnd;
    std::vector<bool> m_keyStates;
    int m_mouseDeltaX;
    int m_mouseDeltaY;
};

