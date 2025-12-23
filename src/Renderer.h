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

struct SpotlightBuffer {
    DirectX::XMFLOAT3 position;
    float range;
    DirectX::XMFLOAT3 direction;
    float spotAngle;
    DirectX::XMFLOAT3 color;
    float intensity;
    DirectX::XMFLOAT2 angles; // x: beam angle, y: field angle
    DirectX::XMFLOAT2 padding;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hwnd);
    void Shutdown();

    void BeginFrame();
    void RenderUI();
    void EndFrame();

    void Log(const std::string& message);

private:
    void InitImGui(HWND hwnd);
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;

    Shader m_basicShader;
    Shader m_debugShader;
    std::unique_ptr<Mesh> m_stageMesh;
    ComPtr<ID3D11Buffer> m_debugBoxVB;
    ComPtr<ID3D11Buffer> m_debugBoxIB;
    DirectX::XMFLOAT3 m_fixturePos;

    SpotlightBuffer m_spotlightData;

    Camera m_camera;
    float m_camDistance;
    float m_camPitch;
    float m_camYaw;
    DirectX::XMFLOAT3 m_camTarget;

    ConstantBuffer<MatrixBuffer> m_matrixBuffer;
    ConstantBuffer<SpotlightBuffer> m_spotlightBuffer;
};
