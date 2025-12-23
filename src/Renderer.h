#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Core/Config.h"
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

__declspec(align(16)) struct MaterialBuffer {
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT4 specParams; // x: intensity, y: shininess, zw: unused
};

struct PointLight {
    DirectX::XMFLOAT4 pos;   // xyz: pos, w: range
    DirectX::XMFLOAT4 color; // xyz: color, w: intensity
};

__declspec(align(16)) struct CeilingLightsBuffer {
    PointLight lights[8];
    DirectX::XMFLOAT4 ambient;
};

__declspec(align(16)) struct FXAABuffer {
    DirectX::XMFLOAT2 rcpFrame;  // 1.0 / screenSize
    DirectX::XMFLOAT2 padding;
};

__declspec(align(16)) struct BlurBuffer {
    DirectX::XMFLOAT2 texelSize;
    DirectX::XMFLOAT2 direction;
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
    Shader m_compositeShader;
    Shader m_fxaaShader;
    std::unique_ptr<Mesh> m_stageMesh;
    std::unique_ptr<Texture> m_goboTexture;
    ComPtr<ID3D11SamplerState> m_samplerState;
    ComPtr<ID3D11SamplerState> m_shadowSampler;
    ComPtr<ID3D11Texture2D> m_shadowMap;
    ComPtr<ID3D11DepthStencilView> m_shadowDSV;
    ComPtr<ID3D11ShaderResourceView> m_shadowSRV;

    ComPtr<ID3D11Buffer> m_debugBoxVB;
    ComPtr<ID3D11Buffer> m_debugBoxIB;
    ComPtr<ID3D11Buffer> m_coneVB;
    ComPtr<ID3D11Buffer> m_coneIB;
    UINT m_coneIndexCount;
    ComPtr<ID3D11Buffer> m_roomVB;
    ComPtr<ID3D11Buffer> m_roomIB;
    ComPtr<ID3D11Buffer> m_sphereVB;
    ComPtr<ID3D11Buffer> m_sphereIB;
    UINT m_sphereIndexCount;
    ComPtr<ID3D11Buffer> m_fullScreenVB;
    DirectX::XMFLOAT3 m_fixturePos;
    float m_stageOffset;

    SpotlightBuffer m_spotlightData;
    VolumetricBuffer m_volumetricData;
    float m_goboShakeAmount;
    bool m_useCMY;
    DirectX::XMFLOAT3 m_cmy;
    
    float m_ceilingLightIntensity;
    float m_ambientFill;
    float m_roomSpecular;
    float m_roomShininess;

    float m_time;
    Camera m_camera;
    float m_camDistance;
    float m_camPitch;
    float m_camYaw;
    DirectX::XMFLOAT3 m_camTarget;

    ConstantBuffer<MatrixBuffer> m_matrixBuffer;
    ConstantBuffer<SpotlightBuffer> m_spotlightBuffer;
    ConstantBuffer<VolumetricBuffer> m_volumetricBuffer;
    ConstantBuffer<MaterialBuffer> m_materialBuffer;
    ConstantBuffer<CeilingLightsBuffer> m_ceilingLightsBuffer;

    ComPtr<ID3D11BlendState> m_additiveBlendState;

    // FXAA resources
    ComPtr<ID3D11Texture2D> m_sceneTexture;
    ComPtr<ID3D11RenderTargetView> m_sceneRTV;
    ComPtr<ID3D11ShaderResourceView> m_sceneSRV;
    ConstantBuffer<FXAABuffer> m_fxaaBuffer;
    bool m_enableFXAA;

    // Volumetric blur resources
    Shader m_blurShader;
    ComPtr<ID3D11Texture2D> m_volTexture;
    ComPtr<ID3D11RenderTargetView> m_volRTV;
    ComPtr<ID3D11ShaderResourceView> m_volSRV;
    ComPtr<ID3D11Texture2D> m_blurTempTexture;
    ComPtr<ID3D11RenderTargetView> m_blurTempRTV;
    ComPtr<ID3D11ShaderResourceView> m_blurTempSRV;
    ConstantBuffer<BlurBuffer> m_blurBuffer;
    bool m_enableVolBlur;
    int m_volBlurPasses;
};
