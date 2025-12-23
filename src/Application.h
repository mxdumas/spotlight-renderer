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

class Application {
public:
    /**
     * @brief Default constructor for the Application class.
     */
    Application() = default;

    /**
     * @brief Destructor for the Application class.
     * Ensures proper cleanup of all application resources.
     */
    ~Application();

    /**
     * @brief Initializes the application, including the graphics device, scene, UI, and render pipeline.
     * 
     * @param hwnd The handle to the window where the application will be rendered.
     * @return true if initialization was successful, false otherwise.
     * @throws std::runtime_exception if critical resources fail to initialize.
     */
    bool Initialize(HWND hwnd);

    /**
     * @brief Shuts down the application and releases all allocated resources.
     */
    void Shutdown();

    /**
     * @brief Begins a new frame by clearing buffers and preparing for rendering.
     */
    void BeginFrame();

    /**
     * @brief Renders the user interface using ImGui.
     */
    void RenderUI();

    /**
     * @brief Ends the current frame by presenting the back buffer and performing any necessary end-of-frame tasks.
     */
    void EndFrame();

    /**
     * @brief Logs a message to the application's logging system.
     * 
     * @param message The string message to be logged.
     */
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

    // Room geometry (GPU resource, owned by Application)
    ComPtr<ID3D11Buffer> m_roomVB;
    ComPtr<ID3D11Buffer> m_roomIB;
};
