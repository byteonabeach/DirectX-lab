#pragma once

#include <windows.h>
#include <string>

class InputDevice;

class Window {
public:
    Window(HINSTANCE hInstance, int nShowCmd, const std::wstring& windowName, int width, int height);
    ~Window();

    bool Initialize();
    int Run();
    HWND GetHandle() const { return m_hWnd; }

    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HINSTANCE m_hInstance;
    HWND m_hWnd;
    int m_nShowCmd;
    std::wstring m_windowName;
    int m_width;
    int m_height;

    InputDevice* m_inputDevice; 
};
