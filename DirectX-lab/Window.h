#pragma once
#include <windows.h>
#include <string>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

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

    bool InitD3D11();
    void Render();
    void Cleanup();

    HINSTANCE m_hInstance;
    HWND m_hWnd;
    int m_nShowCmd;
    std::wstring m_windowName;
    int m_width;
    int m_height;
    InputDevice* m_inputDevice;

    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11DepthStencilView* m_dsv = nullptr;
    ID3D11Texture2D* m_depthStencil = nullptr;
    ID3D11RasterizerState* m_rasterState = nullptr;
};