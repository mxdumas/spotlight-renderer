#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hwnd);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
};
