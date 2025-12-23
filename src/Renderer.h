#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Core/Config.h"
#include "Core/GraphicsDevice.h"
#include "Rendering/RenderPipeline.h"
#include "Geometry/GeometryGenerator.h"
#include "Scene/Scene.h"
#include "UI/UIRenderer.h"
#include <memory>

using Microsoft::WRL::ComPtr;

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool Initialize(HWND hwnd);
    void Shutdown();

    void BeginFrame();
    void RenderUI();
    void EndFrame();

    void Log(const std::string& message);

private:
    // Graphics device (owns device, context, swap chain, back buffer)
    GraphicsDevice m_graphics;

    // Scene (owns camera, lights, meshes, textures)
    Scene m_scene;

    // UI renderer (owns ImGui initialization and rendering)
    UIRenderer m_ui;

    // Render pipeline (owns all render passes and shared resources)
    RenderPipeline m_pipeline;

    // Convenience accessors (delegate to GraphicsDevice)
    ID3D11Device* GetDevice() const { return m_graphics.GetDevice(); }
    ID3D11DeviceContext* GetContext() const { return m_graphics.GetContext(); }

    // Room geometry (GPU resource, owned by Renderer)
    ComPtr<ID3D11Buffer> m_roomVB;
    ComPtr<ID3D11Buffer> m_roomIB;
};
