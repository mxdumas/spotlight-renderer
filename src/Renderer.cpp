#include "Renderer.h"
#include <iostream>
#include <fstream>

Renderer::Renderer()
    : m_stageOffset(0.0f)
    , m_fixturePos(0.0f, Config::Spotlight::DEFAULT_HEIGHT, 0.0f)
    , m_useCMY(false)
    , m_cmy(0.0f, 0.0f, 0.0f)
    , m_roomSpecular(Config::Materials::ROOM_SPECULAR)
    , m_roomShininess(Config::Materials::ROOM_SHININESS)
    , m_time(0.0f)
    , m_camDistance(Config::CameraDefaults::DISTANCE)
    , m_camPitch(Config::CameraDefaults::PITCH)
    , m_camYaw(Config::CameraDefaults::YAW)
    , m_camTarget(0.0f, 0.0f, 0.0f)
{
}

Renderer::~Renderer() {
    Shutdown();
}

void Renderer::Log(const std::string& message) {
    std::ofstream logFile("debug.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
    OutputDebugStringA((message + "\n").c_str());
}

bool Renderer::Initialize(HWND hwnd) {
    // Clear previous log
    { std::ofstream logFile("debug.log", std::ios::trunc); }

    Log("Renderer::Initialize Started");

    // Initialize graphics device (handles device, swap chain, depth buffer)
    Log("Initializing GraphicsDevice...");
    if (!m_graphics.Initialize(hwnd)) {
        Log("GraphicsDevice initialization failed");
        return false;
    }
    Log("GraphicsDevice initialized successfully");

    // Copy pointers to legacy members for backward compatibility during migration
    m_device = m_graphics.GetDevice();
    m_context = m_graphics.GetContext();

    // Initialize render pipeline
    Log("Initializing RenderPipeline...");
    if (!m_pipeline.Initialize(m_device.Get())) {
        Log("RenderPipeline initialization failed");
        return false;
    }
    Log("RenderPipeline initialized successfully");

    // Load Mesh
    Log("Loading stage.obj...");
    m_stageMesh = std::make_unique<Mesh>();
    if (!m_stageMesh->LoadFromOBJ(m_device.Get(), "data/models/stage.obj")) {
        Log("Failed to load stage.obj");
        return false;
    }
    Log("Mesh Loaded Successfully");

    float stageMinY = m_stageMesh->GetMinY();
    m_stageOffset = Config::Room::FLOOR_Y - stageMinY;

    m_fixturePos = { 0.0f, Config::Spotlight::DEFAULT_HEIGHT, 0.0f };
    for (const auto& shape : m_stageMesh->GetShapes()) {
        if (shape.name == "Cylinder.000") {
            m_fixturePos = shape.center;
            break;
        }
    }
    // Apply offset to fixture position as well
    m_fixturePos.y += m_stageOffset;

    // Load gobo texture
    m_goboTexture = std::make_unique<Texture>();
    const char* goboPath = "data/models/gobo.jpg";
    if (!m_goboTexture->LoadFromFile(m_device.Get(), goboPath)) {
        Log("Failed to load gobo.jpg, falling back to stage.png");
        m_goboTexture->LoadFromFile(m_device.Get(), "data/models/stage.png");
    }

    // Create geometry using GeometryGenerator
    Log("Creating geometry...");

    if (!GeometryGenerator::CreateDebugCube(m_device.Get(), m_debugBoxVB, m_debugBoxIB)) {
        Log("Failed to create debug cube");
        return false;
    }
    Log("Debug Cube Created");

    if (!GeometryGenerator::CreateConeProxy(m_device.Get(), m_coneVB, m_coneIB, m_coneIndexCount)) {
        Log("Failed to create cone proxy");
        return false;
    }
    Log("Cone Proxy Created");

    if (!GeometryGenerator::CreateRoomCube(m_device.Get(), m_roomVB, m_roomIB)) {
        Log("Failed to create room cube");
        return false;
    }
    Log("Room Cube Created");

    if (!GeometryGenerator::CreateSphere(m_device.Get(), m_sphereVB, m_sphereIB, m_sphereIndexCount)) {
        Log("Failed to create debug sphere");
        return false;
    }
    Log("Debug Sphere Created");

    // Initialize spotlight
    m_spotlight.SetPosition(m_fixturePos);
    // Point towards center of stage
    DirectX::XMVECTOR lPosVec = DirectX::XMLoadFloat3(&m_fixturePos);
    DirectX::XMVECTOR lDirVec = DirectX::XMVector3Normalize(DirectX::XMVectorNegate(lPosVec));
    DirectX::XMFLOAT3 dir;
    DirectX::XMStoreFloat3(&dir, lDirVec);
    m_spotlight.SetDirection(dir);

    // Initialize Camera
    m_camera.SetPerspective(Config::CameraDefaults::FOV, Config::Display::ASPECT_RATIO,
                            Config::CameraDefaults::CLIP_NEAR, Config::CameraDefaults::CLIP_FAR);

    if (!m_ui.Initialize(hwnd, m_device.Get(), m_context.Get())) {
        Log("ImGui initialization failed");
        return false;
    }
    Log("ImGui Initialized Successfully");

    Log("Renderer::Initialize Completed Successfully");
    return true;
}

void Renderer::Shutdown() {
    // Shutdown UI first
    m_ui.Shutdown();

    // Shutdown pipeline (releases pass resources)
    m_pipeline.Shutdown();

    // Release renderer-specific resources
    m_sphereIB.Reset();
    m_sphereVB.Reset();
    m_coneIB.Reset();
    m_coneVB.Reset();
    m_roomIB.Reset();
    m_roomVB.Reset();
    m_debugBoxIB.Reset();
    m_debugBoxVB.Reset();
    m_stageMesh.reset();
    m_goboTexture.reset();

    // Clear legacy pointers
    m_context.Reset();
    m_device.Reset();

    // Shutdown graphics device (releases device, context, swap chain)
    m_graphics.Shutdown();
}

void Renderer::BeginFrame() {
    m_time += Config::PostProcess::FRAME_DELTA;

    // Start ImGui frame
    m_ui.BeginFrame();

    // Update Camera position
    float camX = m_camDistance * cosf(m_camPitch) * sinf(m_camYaw);
    float camY = m_camDistance * sinf(m_camPitch);
    float camZ = -m_camDistance * cosf(m_camPitch) * cosf(m_camYaw);
    m_camera.SetLookAt({ camX, camY, camZ }, m_camTarget, { 0.0f, 1.0f, 0.0f });

    // Build render context for the pipeline
    RenderContext ctx;
    ctx.camera = &m_camera;
    ctx.cameraPos = { camX, camY, camZ };
    ctx.spotlight = &m_spotlight;
    ctx.ceilingLights = &m_ceilingLights;
    ctx.stageMesh = m_stageMesh.get();
    ctx.goboTexture = m_goboTexture.get();
    ctx.stageOffset = m_stageOffset;
    ctx.time = m_time;
    ctx.roomVB = m_roomVB.Get();
    ctx.roomIB = m_roomIB.Get();
    ctx.roomSpecular = m_roomSpecular;
    ctx.roomShininess = m_roomShininess;
    ctx.depthStencilView = m_graphics.GetDepthStencilView();
    ctx.depthSRV = m_graphics.GetDepthSRV();
    ctx.backBufferRTV = m_graphics.GetBackBufferRTV();

    // Execute the render pipeline
    m_pipeline.Render(m_context.Get(), ctx);
}

void Renderer::RenderUI() {
    // Build UI context with mutable references to scene state
    UIContext ctx;
    ctx.camDistance = &m_camDistance;
    ctx.camPitch = &m_camPitch;
    ctx.camYaw = &m_camYaw;
    ctx.camTarget = &m_camTarget;
    ctx.spotlight = &m_spotlight;
    ctx.fixturePos = &m_fixturePos;
    ctx.useCMY = &m_useCMY;
    ctx.cmy = &m_cmy;
    ctx.ceilingLights = &m_ceilingLights;
    ctx.roomSpecular = &m_roomSpecular;
    ctx.roomShininess = &m_roomShininess;
    ctx.pipeline = &m_pipeline;

    m_ui.RenderControls(ctx);
}

void Renderer::EndFrame() {
    m_ui.EndFrame();
    m_graphics.Present(true);
}
