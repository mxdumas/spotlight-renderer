#include "Renderer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
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

    InitImGui(hwnd);
    Log("ImGui Initialized Successfully");

    Log("Renderer::Initialize Completed Successfully");
    return true;
}

void Renderer::InitImGui(HWND hwnd) {
    Log("InitImGui Started");
    IMGUI_CHECKVERSION();
    Log("ImGui Check Version OK");
    ImGui::CreateContext();
    Log("ImGui Context Created");
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    Log("Initializing ImGui Win32 Backend...");
    if (!ImGui_ImplWin32_Init(hwnd)) {
        Log("ImGui_ImplWin32_Init Failed");
    }
    Log("ImGui Win32 Init Done");

    Log("Initializing ImGui DX11 Backend...");
    if (!m_device || !m_context) {
        Log("CRITICAL: Device or Context is NULL before ImGui DX11 Init!");
    }
    if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get())) {
        Log("ImGui_ImplDX11_Init Failed");
    }
    Log("ImGui DX11 Init Done");
}

void Renderer::Shutdown() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    // Shutdown pipeline first (releases pass resources)
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
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

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
    // Main Window
    ImGui::SetNextWindowPos(ImVec2(Config::UI::WINDOW_POS_X, Config::UI::WINDOW_POS_Y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(Config::UI::WINDOW_WIDTH, Config::UI::WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    ImGui::Begin("Spotlight Renderer Controls");

    // Performance Section
    ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Distance", &m_camDistance, 0.1f, 1.0f, 200.0f);
        ImGui::SliderAngle("Pitch", &m_camPitch, -89.0f, 89.0f);
        ImGui::SliderAngle("Yaw", &m_camYaw, -180.0f, 180.0f);
        ImGui::DragFloat3("Target", &m_camTarget.x, 0.1f);
    }

    if (ImGui::CollapsingHeader("Spotlight Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Get mutable reference to spotlight data for ImGui controls
        SpotlightData& spotData = m_spotlight.GetGPUDataMutable();

        ImGui::Text("Environment");
        float ceilingInt = m_ceilingLights.GetIntensity();
        if (ImGui::SliderFloat("Ceiling Light Intensity", &ceilingInt, 1.0f, 100.0f)) {
            m_ceilingLights.SetIntensity(ceilingInt);
        }
        float ambFill = m_ceilingLights.GetAmbient();
        if (ImGui::SliderFloat("Ambient Fill", &ambFill, 0.0f, 100.0f)) {
            m_ceilingLights.SetAmbient(ambFill);
        }
        ImGui::SliderFloat("Room Specular", &m_roomSpecular, 0.0f, 1.0f);
        ImGui::SliderFloat("Room Shininess", &m_roomShininess, 1.0f, 128.0f);
        ImGui::Separator();

        ImGui::Text("Transform");
        ImGui::DragFloat3("Position", &spotData.posRange.x, 0.1f);
        ImGui::DragFloat3("Direction", &spotData.dirAngle.x, 0.01f);
        if (ImGui::Button("Reset to Fixture")) {
            m_spotlight.SetPosition(m_fixturePos);
        }

        ImGui::Separator();
        ImGui::Text("Color & Intensity");
        ImGui::Checkbox("Use CMY Mixing", &m_useCMY);
        if (m_useCMY) {
            if (ImGui::ColorEdit3("CMY", &m_cmy.x)) {
                m_spotlight.SetColorFromCMY(m_cmy.x, m_cmy.y, m_cmy.z);
            }
        } else {
            ImGui::ColorEdit3("RGB Color", &spotData.colorInt.x);
        }
        ImGui::DragFloat("Intensity", &spotData.colorInt.w, 1.0f, 0.0f, 5000.0f);
        ImGui::DragFloat("Range", &spotData.posRange.w, 1.0f, 10.0f, 1000.0f);

        ImGui::Separator();
        ImGui::Text("Beam Shape");
        ImGui::SliderFloat("Beam Angle", &spotData.coneGobo.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Field Angle", &spotData.coneGobo.y, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Gobo Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        SpotlightData& spotData = m_spotlight.GetGPUDataMutable();
        ImGui::DragFloat("Rotation", &spotData.coneGobo.z, 0.01f);
        float shake = m_spotlight.GetGoboShake();
        if (ImGui::SliderFloat("Shake Amount", &shake, 0.0f, 1.0f)) {
            m_spotlight.SetGoboShake(shake);
        }
    }

    if (ImGui::CollapsingHeader("Volumetric Quality")) {
        VolumetricBuffer& volParams = m_pipeline.GetVolumetricParams();
        ImGui::DragFloat("Step Count", &volParams.params.x, 1.0f, Config::Volumetric::MIN_STEP_COUNT, Config::Volumetric::MAX_STEP_COUNT);
        ImGui::SliderFloat("Density", &volParams.params.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Light Intensity Multiplier", &volParams.params.z, 0.0f, Config::Volumetric::DEFAULT_INTENSITY);
        ImGui::SliderFloat("Anisotropy (G)", &volParams.params.w, Config::Volumetric::MIN_ANISOTROPY, Config::Volumetric::MAX_ANISOTROPY);
    }

    if (ImGui::CollapsingHeader("Post Processing")) {
        bool fxaaEnabled = m_pipeline.IsFXAAEnabled();
        if (ImGui::Checkbox("Enable FXAA", &fxaaEnabled)) {
            m_pipeline.SetFXAAEnabled(fxaaEnabled);
        }
        bool blurEnabled = m_pipeline.IsVolumetricBlurEnabled();
        if (ImGui::Checkbox("Enable Volumetric Blur", &blurEnabled)) {
            m_pipeline.SetVolumetricBlurEnabled(blurEnabled);
        }
        int blurPasses = m_pipeline.GetBlurPasses();
        if (ImGui::SliderInt("Blur Passes", &blurPasses, Config::PostProcess::MIN_BLUR_PASSES, Config::PostProcess::MAX_BLUR_PASSES)) {
            m_pipeline.SetBlurPasses(blurPasses);
        }
    }

    ImGui::End();
}

void Renderer::EndFrame() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_graphics.Present(true);
}
