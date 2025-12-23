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
    DirectX::XMMATRIX invViewProj;
    DirectX::XMFLOAT4 cameraPos;
};

__declspec(align(16)) struct SpotlightBuffer {
    DirectX::XMMATRIX lightViewProj; // c0-c3
    DirectX::XMFLOAT4 posRange;      // c4 (xyz: pos, w: range)
    DirectX::XMFLOAT4 dirAngle;      // c5 (xyz: dir, w: spotAngle)
    DirectX::XMFLOAT4 colorInt;      // c6 (xyz: color, w: intensity)
    DirectX::XMFLOAT4 coneGobo;      // c7 (x: beam, y: field, z: rotation, w: unused)
    DirectX::XMFLOAT4 goboOff;       // c8 (xy: offset, zw: unused)
};

__declspec(align(16)) struct VolumetricBuffer {
    DirectX::XMFLOAT4 params; // x: stepCount, y: density, z: intensity, w: anisotropy (G)
    DirectX::XMFLOAT4 jitter; // x: time, yzw: unused
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
    void RenderShadowMap();
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11ShaderResourceView> m_depthSRV;

    Shader m_basicShader;
    Shader m_debugShader;
    Shader m_shadowShader;
    Shader m_volumetricShader;
    std::unique_ptr<Mesh> m_stageMesh;
    std::unique_ptr<Texture> m_goboTexture;
    ComPtr<ID3D11SamplerState> m_samplerState;
    ComPtr<ID3D11SamplerState> m_shadowSampler;
    ComPtr<ID3D11Texture2D> m_shadowMap;
    ComPtr<ID3D11DepthStencilView> m_shadowDSV;
    ComPtr<ID3D11ShaderResourceView> m_shadowSRV;
    static const int SHADOW_MAP_SIZE = 2048;

    ComPtr<ID3D11Buffer> m_debugBoxVB;
    ComPtr<ID3D11Buffer> m_debugBoxIB;
    ComPtr<ID3D11Buffer> m_fullScreenVB;
    DirectX::XMFLOAT3 m_fixturePos;

    SpotlightBuffer m_spotlightData;
    VolumetricBuffer m_volumetricData;
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
    ConstantBuffer<VolumetricBuffer> m_volumetricBuffer;

    ComPtr<ID3D11BlendState> m_additiveBlendState;
};
