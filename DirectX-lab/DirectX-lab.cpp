#include "Window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    Window window(hInstance, nShowCmd, L"DirectX Pyramid", 1280, 720);
    if (!window.Initialize()) return -1;
    return window.Run();
}