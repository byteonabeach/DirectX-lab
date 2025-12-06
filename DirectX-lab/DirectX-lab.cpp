#include "Window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    Window window(hInstance, nShowCmd, L"My DirectX Lab4 ITMO", 800, 600);

    if (!window.Initialize()) return 1;

    return window.Run();
}