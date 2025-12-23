#pragma once

#include <d3d11.h>
#include <windows.h>
#include <DirectXMath.h>

// Forward declarations
class Scene;
class RenderPipeline;

// UI context passed to render controls
struct UIContext {
    Scene* scene;
    RenderPipeline* pipeline;
};

// Handles all ImGui initialization, rendering, and shutdown
// Extracted from Renderer to separate UI concerns from rendering
class UIRenderer {
public:
    UIRenderer() = default;
    ~UIRenderer();

    // Initialize ImGui backends (Win32 + DX11)
    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);

    // Cleanup ImGui
    void Shutdown();

    // Start a new ImGui frame (call at beginning of frame)
    void BeginFrame();

    // Render all UI controls (call after BeginFrame, before EndFrame)
    void RenderControls(UIContext& ctx);

    // Finalize and render ImGui draw data (call at end of frame)
    void EndFrame();

private:
    bool m_initialized = false;
};
