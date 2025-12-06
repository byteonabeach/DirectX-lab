#include "Window.h"
#include "InputDevice.h" 

Window::Window(HINSTANCE hInstance, int nShowCmd, const std::wstring& windowName, int width, int height)
    : m_hInstance(hInstance), m_nShowCmd(nShowCmd), m_windowName(windowName), m_width(width), m_height(height), m_hWnd(nullptr), m_inputDevice(nullptr) {
}

Window::~Window() {
    if (m_inputDevice) 
        delete m_inputDevice;

    if (m_hWnd)
        DestroyWindow(m_hWnd);
}

bool Window::Initialize() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"MyWindowClass";
    wc.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

    if (!RegisterClassEx(&wc)) return false;

    RECT windowRect = { 0, 0, m_width, m_height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"MyWindowClass",
        m_windowName.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );

    if (!m_hWnd) return false;

    ShowWindow(m_hWnd, m_nShowCmd);
    UpdateWindow(m_hWnd);

    m_inputDevice = new InputDevice(m_hWnd);
    if (!m_inputDevice->Initialize()) return false;

    return true;
}

int Window::Run() {
    MSG msg = {};
    bool isExitRequested = false;

    while (!isExitRequested) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) isExitRequested = true;
        }

        m_inputDevice->Update();
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pWindow = nullptr;

    if (uMsg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    } else {
        LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pWindow = reinterpret_cast<Window*>(ptr);
    }

    if (pWindow) return pWindow->HandleMessage(uMsg, wParam, lParam);

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;

    case WM_INPUT:
        m_inputDevice->ProcessRawInput(lParam);
        return 0;

    default:
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }
}