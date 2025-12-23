#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Core/Config.h"
#include "Core/GraphicsDevice.h"
#include "Rendering/RenderPipeline.h"
#include "Geometry/GeometryGenerator.h"
#include "Scene/Spotlight.h"
#include "Scene/CeilingLights.h"
#include "Mesh.h"
#include "Camera.h"
#include "Texture.h"
#include <memory>

using Microsoft::WRL::ComPtr;

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

    // Graphics device (owns device, context, swap chain, back buffer)
    GraphicsDevice m_graphics;

    // Render pipeline (owns all render passes and shared resources)
    RenderPipeline m_pipeline;

    // Convenience accessors (delegate to GraphicsDevice)
    ID3D11Device* GetDevice() const { return m_graphics.GetDevice(); }
    ID3D11DeviceContext* GetContext() const { return m_graphics.GetContext(); }

    // Legacy pointers kept for gradual migration
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;

    // Scene resources
    std::unique_ptr<Mesh> m_stageMesh;
    std::unique_ptr<Texture> m_goboTexture;
    DirectX::XMFLOAT3 m_fixturePos;
    float m_stageOffset;

    // Room geometry (owned by Renderer, passed to pipeline)
    ComPtr<ID3D11Buffer> m_roomVB;
    ComPtr<ID3D11Buffer> m_roomIB;

    // Debug geometry (kept for future use)
    ComPtr<ID3D11Buffer> m_debugBoxVB;
    ComPtr<ID3D11Buffer> m_debugBoxIB;
    ComPtr<ID3D11Buffer> m_coneVB;
    ComPtr<ID3D11Buffer> m_coneIB;
    UINT m_coneIndexCount;
    ComPtr<ID3D11Buffer> m_sphereVB;
    ComPtr<ID3D11Buffer> m_sphereIB;
    UINT m_sphereIndexCount;

    // Scene components
    Spotlight m_spotlight;
    CeilingLights m_ceilingLights;
    bool m_useCMY;
    DirectX::XMFLOAT3 m_cmy;

    float m_roomSpecular;
    float m_roomShininess;

    float m_time;
    Camera m_camera;
    float m_camDistance;
    float m_camPitch;
    float m_camYaw;
    DirectX::XMFLOAT3 m_camTarget;
};
