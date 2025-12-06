#include "InputDevice.h"
#include <hidsdi.h> 
#include <iostream>

InputDevice::InputDevice(HWND hWnd) : m_hWnd(hWnd), m_mouseDeltaX(0), m_mouseDeltaY(0) {
    m_keyStates.resize(256, false);
}

InputDevice::~InputDevice() {}

bool InputDevice::Initialize() {
    RAWINPUTDEVICE rid[2];

    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x06;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = m_hWnd;

    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x02;
    rid[1].dwFlags = 0;
    rid[1].hwndTarget = m_hWnd;

    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) return false;

    return true;
}

void InputDevice::Update() {
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
}

void InputDevice::ProcessRawInput(LPARAM lParam) {
    UINT dwSize = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

    if (dwSize == 0) return;

    std::vector<BYTE> buffer(dwSize);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) return;

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());

    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        USHORT vKey = raw->data.keyboard.VKey;
        bool isDown = (raw->data.keyboard.Flags & RI_KEY_BREAK) == 0;

        if (vKey < m_keyStates.size()) m_keyStates[vKey] = isDown;
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE) {
        m_mouseDeltaX += raw->data.mouse.lLastX;
        m_mouseDeltaY += raw->data.mouse.lLastY;
    }
}

bool InputDevice::IsKeyDown(int vKey) const {
    if (vKey < 0 || vKey >= m_keyStates.size()) return false;
    return m_keyStates[vKey];
}
