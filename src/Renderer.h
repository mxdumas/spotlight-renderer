#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "ConstantBuffer.h"
#include <memory>

using Microsoft::WRL::ComPtr;

struct MatrixBuffer {
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

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

    Camera m_camera;
    ConstantBuffer<MatrixBuffer> m_matrixBuffer;
};
