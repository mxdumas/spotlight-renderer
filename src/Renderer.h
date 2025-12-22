#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Shader.h"
#include "Mesh.h"
#include <memory>

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

    Shader m_basicShader;
    std::unique_ptr<Mesh> m_stageMesh;
};
