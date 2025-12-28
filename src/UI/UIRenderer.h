#pragma once

#include <windows.h>
#include <DirectXMath.h>
#include <d3d11.h>

// Forward declarations
class Scene;
class RenderPipeline;

/**
 * @struct UIContext
 * @brief Context data required for rendering UI controls.
 *
 * Provides the UI renderer with access to the scene and render pipeline for configuration.
 */
struct UIContext
{
    Scene *scene;             ///< Pointer to the scene for camera and light controls.
    RenderPipeline *pipeline; ///< Pointer to the render pipeline for post-processing controls.
};

/**
 * @class UIRenderer
 * @brief Handles ImGui initialization, rendering, and shutdown.
 *
 * This class isolates UI concerns from the main rendering logic, managing the lifecycle
 * of the ImGui context and providing methods to render application-specific controls.
 */
class UIRenderer
{
public:
    /**
     * @brief Default constructor for the UIRenderer class.
     */
    UIRenderer() = default;

    /**
     * @brief Destructor for the UIRenderer class.
     * Ensures ImGui is properly shut down if initialized.
     */
    ~UIRenderer();

    /**
     * @brief Initializes the ImGui backends for Win32 and DirectX 11.
     *
     * @param hwnd Handle to the window.
     * @param device Pointer to the ID3D11Device.
     * @param context Pointer to the ID3D11DeviceContext.
     * @return true if initialization was successful, false otherwise.
     */
    bool Initialize(HWND hwnd, ID3D11Device *device, ID3D11DeviceContext *context);

    /**
     * @brief Shuts down ImGui and cleans up backend resources.
     */
    void Shutdown();

    /**
     * @brief Starts a new ImGui frame.
     * Should be called at the beginning of each frame.
     */
    void BeginFrame();

    /**
     * @brief Renders the application's UI controls.
     *
     * @param ctx The UIContext providing access to scene and pipeline state.
     */
    void RenderControls(UIContext &ctx);

    /**
     * @brief Finalizes the ImGui frame and renders draw data to the back buffer.
     * Should be called at the end of each frame, after all UI controls are rendered.
     */
    void EndFrame();

private:
    bool m_initialized = false;
};
