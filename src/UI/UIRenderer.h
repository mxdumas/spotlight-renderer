#pragma once

#include <d3d11.h>
#include <windows.h>
#include <DirectXMath.h>

// Forward declarations
class Spotlight;
class CeilingLights;
class RenderPipeline;

// UI context passed to render controls
// Contains mutable references to scene state that the UI can modify
struct UIContext {
    // Camera
    float* camDistance;
    float* camPitch;
    float* camYaw;
    DirectX::XMFLOAT3* camTarget;

    // Spotlight
    Spotlight* spotlight;
    const DirectX::XMFLOAT3* fixturePos; // For "Reset to Fixture" button

    // CMY mixing state
    bool* useCMY;
    DirectX::XMFLOAT3* cmy;

    // Environment
    CeilingLights* ceilingLights;
    float* roomSpecular;
    float* roomShininess;

    // Pipeline settings
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
