#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "ConstantBuffer.h"
#include "Texture.h"
#include <memory>

using Microsoft::WRL::ComPtr;

__declspec(align(16)) struct MatrixBuffer {
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

__declspec(align(16)) struct SpotlightBuffer {
    DirectX::XMMATRIX lightViewProj; // c0-c3
    DirectX::XMFLOAT4 posRange;      // c4 (xyz: pos, w: range)
    DirectX::XMFLOAT4 dirAngle;      // c5 (xyz: dir, w: spotAngle)
    DirectX::XMFLOAT4 colorInt;      // c6 (xyz: color, w: intensity)
    DirectX::XMFLOAT4 coneGobo;      // c7 (x: beam, y: field, z: rotation, w: unused)
    DirectX::XMFLOAT4 goboOff;       // c8 (xy: offset, zw: unused)
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
    std::unique_ptr<Texture> m_goboTexture;
    ComPtr<ID3D11SamplerState> m_samplerState;
    ComPtr<ID3D11Buffer> m_debugBoxVB;
    ComPtr<ID3D11Buffer> m_debugBoxIB;
    DirectX::XMFLOAT3 m_fixturePos;

    SpotlightBuffer m_spotlightData;
    float m_goboShakeAmount;
    bool m_useCMY;
    DirectX::XMFLOAT3 m_cmy;

    float m_time;
    Camera m_camera;
    float m_camDistance;
    float m_camPitch;
    float m_camYaw;
    DirectX::XMFLOAT3 m_camTarget;

    ConstantBuffer<MatrixBuffer> m_matrixBuffer;
    ConstantBuffer<SpotlightBuffer> m_spotlightBuffer;
};
