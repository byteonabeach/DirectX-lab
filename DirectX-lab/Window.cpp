#include "Window.h"
#include "InputDevice.h"
#include <vector>

using namespace DirectX;

struct Vertex { XMFLOAT3 pos; XMFLOAT4 color; };
struct ConstantBuffer { XMMATRIX worldViewProj; };

Window::Window(HINSTANCE hInstance, int nShowCmd, const std::wstring& windowName, int width, int height)
    : m_hInstance(hInstance), m_nShowCmd(nShowCmd), m_windowName(windowName), m_width(width), m_height(height),
    m_hWnd(nullptr), m_inputDevice(nullptr) {
}

Window::~Window() {
    Cleanup();
    if (m_inputDevice) delete m_inputDevice;
    if (m_hWnd) DestroyWindow(m_hWnd);
}

bool Window::Initialize() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"DXWindowClass";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassEx(&wc);

    RECT rc = { 0, 0, m_width, m_height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    m_hWnd = CreateWindowEx(WS_EX_APPWINDOW, L"DXWindowClass", m_windowName.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, m_hInstance, this);

    if (!m_hWnd) return false;

    ShowWindow(m_hWnd, m_nShowCmd);
    UpdateWindow(m_hWnd);

    m_inputDevice = new InputDevice(m_hWnd);
    m_inputDevice->Initialize();

    return InitD3D11();
}

bool Window::InitD3D11() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = m_width;
    sd.BufferDesc.Height = m_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &featureLevel, &m_context);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencil);
    m_device->CreateDepthStencilView(m_depthStencil, nullptr, &m_dsv);

    m_context->OMSetRenderTargets(1, &m_rtv, m_dsv);

    D3D11_VIEWPORT vp = {};
    vp.Width = (FLOAT)m_width;
    vp.Height = (FLOAT)m_height;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = TRUE;
    m_device->CreateRasterizerState(&rsDesc, &m_rasterState);
    m_context->RSSetState(m_rasterState);

    const char* vsSource =
        "cbuffer ConstantBuffer : register(b0) { float4x4 wvp; }"
        "struct VS_OUT { float4 pos : SV_POSITION; float4 col : COLOR; };"
        "VS_OUT main(float3 pos : POSITION, float4 col : COLOR) {"
        "    VS_OUT o; o.pos = mul(float4(pos, 1.0), wvp); o.col = col; return o; }";

    const char* psSource =
        "float4 main(float4 col : COLOR) : SV_TARGET { return col; }";

    ID3DBlob* vsBlob;
    ID3DBlob* psBlob;
    D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, nullptr);

    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();

    Vertex pyramid[] = {
        {{ 0.0f,  1.0f,  0.0f}, {1, 0, 0, 1}},
        {{ 1.0f, -1.0f,  1.0f}, {0, 1, 0, 1}},
        {{-1.0f, -1.0f,  1.0f}, {0, 0, 1, 1}},
        {{ 1.0f, -1.0f, -1.0f}, {1, 1, 0, 1}},
        {{-1.0f, -1.0f, -1.0f}, {1, 0, 1, 1}}
    };

    UINT indices[] = {
        0, 2, 1,
        0, 1, 3,
        0, 3, 4,
        0, 4, 2,
        1, 2, 4, 1, 4, 3
    };

    std::vector<Vertex> vertices;
    std::vector<UINT> indexList;
    for (UINT i = 0; i < 18; i += 3) {
        vertices.push_back(pyramid[indices[i]]);
        vertices.push_back(pyramid[indices[i + 1]]);
        vertices.push_back(pyramid[indices[i + 2]]);
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = { vertices.data() };
    m_device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);

    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetShader(m_ps, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

    return true;
}

void Window::Render() {
    float clearColor[] = { 0.1f, 0.1f, 0.2f, 1.0f };
    m_context->ClearRenderTargetView(m_rtv, clearColor);
    m_context->ClearDepthStencilView(m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    XMMATRIX world = XMMatrixRotationY((float)GetTickCount64() / 1000.0f);
    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 2, -5, 0), XMVectorZero(), XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_width / m_height, 0.1f, 100.0f);
    XMMATRIX wvp = XMMatrixTranspose(world * view * proj);

    D3D11_MAPPED_SUBRESOURCE ms;
    m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    *(ConstantBuffer*)ms.pData = { wvp };
    m_context->Unmap(m_constantBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(18, 0);

    m_swapChain->Present(1, 0);
}

void Window::Cleanup() {
    if (m_swapChain) m_swapChain->SetFullscreenState(FALSE, nullptr);
    if (m_rasterState) m_rasterState->Release();
    if (m_depthStencil) m_depthStencil->Release();
    if (m_dsv) m_dsv->Release();
    if (m_constantBuffer) m_constantBuffer->Release();
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_ps) m_ps->Release();
    if (m_vs) m_vs->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_rtv) m_rtv->Release();
    if (m_context) m_context->Release();
    if (m_device) m_device->Release();
    if (m_swapChain) m_swapChain->Release();
}

int Window::Run() {
    MSG msg = {};

    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) return (int)msg.wParam;
        }

        m_inputDevice->Update();
        if (m_inputDevice->IsKeyDown(VK_ESCAPE)) PostQuitMessage(0);
    
        Render();
    }

    return (int) msg.wParam;
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pThis = nullptr;
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (Window*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
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
