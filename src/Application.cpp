#include "Application.h"
#include <iostream>
#include <fstream>

Application::~Application() {
    Shutdown();
}

void Application::Log(const std::string& message) {
    std::ofstream logFile("debug.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
    OutputDebugStringA((message + "\n").c_str());
}

bool Application::Initialize(HWND hwnd) {
    // Clear previous log
    { std::ofstream logFile("debug.log", std::ios::trunc); }

    Log("Application::Initialize Started");

    // Initialize graphics device (handles device, swap chain, depth buffer)
    Log("Initializing GraphicsDevice...");
    if (!m_graphics.Initialize(hwnd)) {
        Log("GraphicsDevice initialization failed");
        return false;
    }
    Log("GraphicsDevice initialized successfully");

    // Initialize render pipeline
    Log("Initializing RenderPipeline...");
    if (!m_pipeline.Initialize(m_graphics.GetDevice())) {
        Log("RenderPipeline initialization failed");
        return false;
    }
    Log("RenderPipeline initialized successfully");

    // Initialize scene (loads meshes, textures, sets up camera and lights)
    Log("Initializing Scene...");
    if (!m_scene.Initialize(m_graphics.GetDevice())) {
        Log("Scene initialization failed");
        return false;
    }
    Log("Scene initialized successfully");

    // Create room geometry
    Log("Creating room geometry...");
    if (!GeometryGenerator::CreateRoomCube(m_graphics.GetDevice(), m_roomVB, m_roomIB)) {
        Log("Failed to create room cube");
        return false;
    }
    Log("Room Cube Created");

    // Initialize UI
    if (!m_ui.Initialize(hwnd, m_graphics.GetDevice(), m_graphics.GetContext())) {
        Log("ImGui initialization failed");
        return false;
    }
    Log("ImGui Initialized Successfully");

    Log("Application::Initialize Completed Successfully");
    return true;
}

void Application::Shutdown() {
    // Shutdown UI first
    m_ui.Shutdown();

    // Shutdown pipeline (releases pass resources)
    m_pipeline.Shutdown();

    // Release room geometry
    m_roomIB.Reset();
    m_roomVB.Reset();

    // Shutdown graphics device (releases device, context, swap chain)
    m_graphics.Shutdown();
}

void Application::BeginFrame() {
    // Update scene
    m_scene.Update(Config::PostProcess::FRAME_DELTA);
    m_scene.UpdateCamera();

    // Start ImGui frame
    m_ui.BeginFrame();

    // Build render context for the pipeline
    RenderContext ctx;
    ctx.camera = &m_scene.GetCamera();
    ctx.cameraPos = m_scene.GetCameraPosition();
    ctx.anchorPositions = m_scene.GetAnchorPositions();
    ctx.spotlight = &m_scene.GetSpotlight();
    ctx.ceilingLights = &m_scene.GetCeilingLights();
    ctx.stageMesh = m_scene.GetStageMesh();
    ctx.goboTexture = m_scene.GetGoboTexture();
    ctx.stageOffset = m_scene.GetStageOffset();
    ctx.time = m_scene.GetTime();
    ctx.roomVB = m_roomVB.Get();
    ctx.roomIB = m_roomIB.Get();
    ctx.roomSpecular = m_scene.GetRoomSpecular();
    ctx.roomShininess = m_scene.GetRoomShininess();
    ctx.depthStencilView = m_graphics.GetDepthStencilView();
    ctx.depthSRV = m_graphics.GetDepthSRV();
    ctx.backBufferRTV = m_graphics.GetBackBufferRTV();

    // Execute the render pipeline
    m_pipeline.Render(m_graphics.GetContext(), ctx);
}

void Application::RenderUI() {
    UIContext ctx;
    ctx.scene = &m_scene;
    ctx.pipeline = &m_pipeline;

    m_ui.RenderControls(ctx);
}

void Application::EndFrame() {
    m_ui.EndFrame();
    m_graphics.Present(true);
}
