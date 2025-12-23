#include "Renderer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <iostream>
#include <fstream>

Renderer::Renderer() {
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
    // These will be removed once all code uses GraphicsDevice directly
    m_device = m_graphics.GetDevice();
    m_context = m_graphics.GetContext();
    m_swapChain = m_graphics.GetSwapChain();
    m_renderTargetView = m_graphics.GetBackBufferRTV();
    m_depthStencilView = m_graphics.GetDepthStencilView();
    m_depthSRV = m_graphics.GetDepthSRV();

    Log("Viewport and Render Targets Set");

    // HRESULT for remaining resource creation
    HRESULT hr;

    // Initialize shader
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    Log("Loading basic.hlsl...");
    if (!m_basicShader.LoadVertexShader(m_device.Get(), L"shaders/basic.hlsl", "VS", layout)) {
        Log("Failed to load vertex shader basic.hlsl");
        return false;
    }
    if (!m_basicShader.LoadPixelShader(m_device.Get(), L"shaders/basic.hlsl", "PS")) {
        Log("Failed to load pixel shader basic.hlsl");
        return false;
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> debugLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Log("Loading debug.hlsl...");
    if (!m_debugShader.LoadVertexShader(m_device.Get(), L"shaders/debug.hlsl", "VS", debugLayout)) return false;
    if (!m_debugShader.LoadPixelShader(m_device.Get(), L"shaders/debug.hlsl", "PS")) return false;
    Log("Shaders Loaded Successfully");

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

    m_goboTexture = std::make_unique<Texture>();
    const char* goboPath = "data/models/gobo.jpg";
    if (!m_goboTexture->LoadFromFile(m_device.Get(), goboPath)) {
        Log("Failed to load gobo.jpg, falling back to stage.png");
        m_goboTexture->LoadFromFile(m_device.Get(), "data/models/stage.png");
    }

    // Create Sampler State
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.BorderColor[0] = 0.0f;
    sampDesc.BorderColor[1] = 0.0f;
    sampDesc.BorderColor[2] = 0.0f;
    sampDesc.BorderColor[3] = 0.0f;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_device->CreateSamplerState(&sampDesc, &m_samplerState);
    if (FAILED(hr)) return false;

    // Create Shadow Mapping Resources
    D3D11_TEXTURE2D_DESC smDesc = {};
    smDesc.Width = Config::Shadow::MAP_SIZE;
    smDesc.Height = Config::Shadow::MAP_SIZE;
    smDesc.MipLevels = 1;
    smDesc.ArraySize = 1;
    smDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    smDesc.SampleDesc.Count = 1;
    smDesc.Usage = D3D11_USAGE_DEFAULT;
    smDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    hr = m_device->CreateTexture2D(&smDesc, nullptr, &m_shadowMap);
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC sdsvDesc = {};
    sdsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    sdsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = m_device->CreateDepthStencilView(m_shadowMap.Get(), &sdsvDesc, &m_shadowDSV);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC ssrvDesc = {};
    ssrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    ssrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ssrvDesc.Texture2D.MipLevels = 1;
    hr = m_device->CreateShaderResourceView(m_shadowMap.Get(), &ssrvDesc, &m_shadowSRV);
    if (FAILED(hr)) return false;

    D3D11_SAMPLER_DESC shadowSampDesc = {};
    shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    shadowSampDesc.BorderColor[0] = 1.0f;
    shadowSampDesc.BorderColor[1] = 1.0f;
    shadowSampDesc.BorderColor[2] = 1.0f;
    shadowSampDesc.BorderColor[3] = 1.0f;
    shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    hr = m_device->CreateSamplerState(&shadowSampDesc, &m_shadowSampler);
    if (FAILED(hr)) return false;

    Log("Loading shadow.hlsl...");
    if (!m_shadowShader.LoadVertexShader(m_device.Get(), L"shaders/shadow.hlsl", "VS", layout)) return false;
    if (!m_shadowShader.LoadPixelShader(m_device.Get(), L"shaders/shadow.hlsl", "PS")) return false;

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

    if (!GeometryGenerator::CreateFullScreenQuad(m_device.Get(), m_fullScreenVB)) {
        Log("Failed to create fullscreen quad");
        return false;
    }
    Log("Full Screen Quad Created");

    std::vector<D3D11_INPUT_ELEMENT_DESC> fsLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Log("Loading volumetric.hlsl...");
    if (!m_volumetricShader.LoadVertexShader(m_device.Get(), L"shaders/volumetric.hlsl", "VS", fsLayout)) { Log("Failed to load volumetric VS"); return false; }
    if (!m_volumetricShader.LoadPixelShader(m_device.Get(), L"shaders/volumetric.hlsl", "PS")) { Log("Failed to load volumetric PS"); return false; }

    Log("Loading composite.hlsl...");
    if (!m_compositeShader.LoadVertexShader(m_device.Get(), L"shaders/composite.hlsl", "VS", fsLayout)) { Log("Failed to load composite VS"); return false; }
    if (!m_compositeShader.LoadPixelShader(m_device.Get(), L"shaders/composite.hlsl", "PS")) { Log("Failed to load composite PS"); return false; }
    Log("Volumetric and Composite Shaders Loaded");

    Log("Loading fxaa.hlsl...");
    if (!m_fxaaShader.LoadVertexShader(m_device.Get(), L"shaders/fxaa.hlsl", "VS", fsLayout)) { Log("Failed to load FXAA VS"); return false; }
    if (!m_fxaaShader.LoadPixelShader(m_device.Get(), L"shaders/fxaa.hlsl", "PS")) { Log("Failed to load FXAA PS"); return false; }
    Log("FXAA Shader Loaded");

    Log("Loading blur.hlsl...");
    if (!m_blurShader.LoadVertexShader(m_device.Get(), L"shaders/blur.hlsl", "VS", fsLayout)) { Log("Failed to load Blur VS"); return false; }
    if (!m_blurShader.LoadPixelShader(m_device.Get(), L"shaders/blur.hlsl", "PS")) { Log("Failed to load Blur PS"); return false; }
    Log("Blur Shader Loaded");

    // Initialize spotlight
    m_spotlight.SetPosition(m_fixturePos);
    // Point towards center of stage
    DirectX::XMVECTOR lPosVec = DirectX::XMLoadFloat3(&m_fixturePos);
    DirectX::XMVECTOR lDirVec = DirectX::XMVector3Normalize(DirectX::XMVectorNegate(lPosVec));
    DirectX::XMFLOAT3 dir;
    DirectX::XMStoreFloat3(&dir, lDirVec);
    m_spotlight.SetDirection(dir);

    m_volumetricData.params = { Config::Volumetric::DEFAULT_STEP_COUNT, Config::Volumetric::DEFAULT_DENSITY,
                                 Config::Volumetric::DEFAULT_INTENSITY, Config::Volumetric::DEFAULT_ANISOTROPY };
    m_volumetricData.jitter = { 0.0f, 0.0f, 0.0f, 0.0f };

    m_time = 0.0f;
    m_useCMY = false;
    m_cmy = { 0.0f, 0.0f, 0.0f };
    m_roomSpecular = Config::Materials::ROOM_SPECULAR;
    m_roomShininess = Config::Materials::ROOM_SHININESS;

    // Initialize Camera
    m_camDistance = Config::CameraDefaults::DISTANCE;
    m_camPitch = Config::CameraDefaults::PITCH;
    m_camYaw = Config::CameraDefaults::YAW;
    m_camTarget = { 0.0f, 0.0f, 0.0f };
    m_camera.SetPerspective(Config::CameraDefaults::FOV, Config::Display::ASPECT_RATIO, Config::CameraDefaults::CLIP_NEAR, Config::CameraDefaults::CLIP_FAR);

    // Initialize Constant Buffers
    if (!m_matrixBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize matrix constant buffer");
        return false;
    }
    if (!m_spotlightBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize spotlight constant buffer");
        return false;
    }
    if (!m_volumetricBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize volumetric constant buffer");
        return false;
    }
    if (!m_materialBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize material constant buffer");
        return false;
    }
    if (!m_ceilingLightsBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize ceiling lights constant buffer");
        return false;
    }
    if (!m_fxaaBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize FXAA constant buffer");
        return false;
    }

    // Create render targets using RenderTarget class
    if (!m_sceneRT.Create(m_device.Get(), Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT)) {
        Log("Failed to create scene render target");
        return false;
    }
    Log("Scene render target created");

    if (!m_volRT.Create(m_device.Get(), Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT)) {
        Log("Failed to create volumetric render target");
        return false;
    }
    Log("Volumetric render target created");

    if (!m_blurTempRT.Create(m_device.Get(), Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT)) {
        Log("Failed to create blur temp render target");
        return false;
    }
    Log("Blur temp render target created");

    if (!m_blurBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize blur constant buffer");
        return false;
    }

    m_enableFXAA = true;
    m_enableVolBlur = true;
    m_volBlurPasses = Config::PostProcess::DEFAULT_BLUR_PASSES;

    // Create Blend State
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&blendDesc, &m_additiveBlendState);
    if (FAILED(hr)) return false;

    InitImGui(hwnd);
    Log("ImGui Initialized Successfully");

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

    // Release renderer-specific resources
    m_shadowSRV.Reset();
    m_shadowDSV.Reset();
    m_shadowMap.Reset();
    m_shadowSampler.Reset();
    m_additiveBlendState.Reset();
    m_fullScreenVB.Reset();
    m_debugBoxIB.Reset();
    m_debugBoxVB.Reset();
    m_stageMesh.reset();
    m_goboTexture.reset();

    // Clear legacy pointers (they point to GraphicsDevice resources)
    m_depthSRV.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();

    // Shutdown graphics device (releases device, context, swap chain)
    m_graphics.Shutdown();
}

void Renderer::RenderShadowMap() {
    m_context->ClearDepthStencilView(m_shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_context->OMSetRenderTargets(0, nullptr, m_shadowDSV.Get());

    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(Config::Shadow::MAP_SIZE);
    vp.Height = static_cast<float>(Config::Shadow::MAP_SIZE);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    // Get spotlight data for shadow rendering
    const SpotlightData& spotData = m_spotlight.GetGPUData();
    DirectX::XMVECTOR lPos = DirectX::XMVectorSet(spotData.posRange.x, spotData.posRange.y, spotData.posRange.z, 1.0f);
    DirectX::XMVECTOR lDir = DirectX::XMVector3Normalize(DirectX::XMVectorSet(spotData.dirAngle.x, spotData.dirAngle.y, spotData.dirAngle.z, 0.0f));
    DirectX::XMVECTOR lUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(DirectX::XMVectorGetY(lDir)) > 0.99f) lUp = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    DirectX::XMMATRIX lView = DirectX::XMMatrixLookToLH(lPos, lDir, lUp);
    DirectX::XMMATRIX lProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1.0f, 0.1f, spotData.posRange.w);

    MatrixBuffer mb;
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0.0f, m_stageOffset, 0.0f));
    mb.view = DirectX::XMMatrixTranspose(lView);
    mb.projection = DirectX::XMMatrixTranspose(lProj);
    mb.invViewProj = DirectX::XMMatrixIdentity();
    mb.cameraPos = { 0,0,0,0 };
    m_matrixBuffer.Update(m_context.Get(), mb);

    m_context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    
    m_shadowShader.Bind(m_context.Get());
    if (m_stageMesh) {
        m_stageMesh->Draw(m_context.Get());
    }

    // Restore viewport and render targets is handled in BeginFrame
}

void Renderer::BeginFrame() {
    m_time += Config::PostProcess::FRAME_DELTA;
    
    // Unbind all SRVs to avoid conflicts with depth/stencil and render targets
    ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
    m_context->PSSetShaderResources(0, 8, nullSRVs);

    static bool firstFrame = true;
    if (firstFrame) Log("First BeginFrame Started");

    // Update spotlight
    m_spotlight.UpdateLightMatrix();
    m_spotlight.UpdateGoboShake(m_time);

    // Render Shadow Map
    RenderShadowMap();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    if (firstFrame) Log("ImGui NewFrame Done");

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // Render to scene texture (for FXAA), not directly to swap chain
    m_sceneRT.Bind(m_context.Get(), m_depthStencilView.Get());
    m_sceneRT.Clear(m_context.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(Config::Display::WINDOW_WIDTH);
    viewport.Height = static_cast<float>(Config::Display::WINDOW_HEIGHT);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    m_context->RSSetViewports(1, &viewport);

    if (firstFrame) Log("Clear Views Done");

    // Update Camera
    float camX = m_camDistance * cosf(m_camPitch) * sinf(m_camYaw);
    float camY = m_camDistance * sinf(m_camPitch);
    float camZ = -m_camDistance * cosf(m_camPitch) * cosf(m_camYaw);
    m_camera.SetLookAt({ camX, camY, camZ }, m_camTarget, { 0.0f, 1.0f, 0.0f });

    // Update matrices for main pass
    MatrixBuffer mb;
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    mb.view = DirectX::XMMatrixTranspose(m_camera.GetViewMatrix());
    mb.projection = DirectX::XMMatrixTranspose(m_camera.GetProjectionMatrix());
    
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, m_camera.GetViewMatrix());
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(nullptr, m_camera.GetProjectionMatrix());
    // To transform from Clip Space to World Space: World = Clip * InvProj * InvView
    // But since we transpose for the constant buffer, we calculate (InvProj * InvView)^T
    mb.invViewProj = DirectX::XMMatrixTranspose(invProj * invView);

    mb.cameraPos = { camX, camY, camZ, 1.0f };

    m_matrixBuffer.Update(m_context.Get(), mb);
    m_spotlightBuffer.Update(m_context.Get(), m_spotlight.GetGPUData());

    // Update Ceiling Lights
    m_ceilingLights.Update();
    m_ceilingLightsBuffer.Update(m_context.Get(), m_ceilingLights.GetGPUData());

    if (firstFrame) Log("Buffers Updated");

    m_context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(1, 1, m_spotlightBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(2, 1, m_materialBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(3, 1, m_ceilingLightsBuffer.GetAddressOf());
    
    if (m_goboTexture) {
        ID3D11ShaderResourceView* srvs[] = { m_goboTexture->GetSRV(), m_shadowSRV.Get() };
        m_context->PSSetShaderResources(0, 2, srvs);
        
        ID3D11SamplerState* samplers[] = { m_samplerState.Get(), m_shadowSampler.Get() };
        m_context->PSSetSamplers(0, 2, samplers);
    }

    m_basicShader.Bind(m_context.Get());

    // Render Room (Background)
    {
        // Dark Gray Material
        MaterialBuffer mb = {};
        mb.color = { Config::Materials::ROOM_COLOR, Config::Materials::ROOM_COLOR, Config::Materials::ROOM_COLOR, 1.0f };
        mb.specParams = { m_roomSpecular, m_roomShininess, 0.0f, 0.0f };
        m_materialBuffer.Update(m_context.Get(), mb);

        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        ComPtr<ID3D11RasterizerState> rs;
        m_device->CreateRasterizerState(&rd, &rs);
        m_context->RSSetState(rs.Get());

        UINT stride = Config::Vertex::STRIDE_FULL;
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_roomVB.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(m_roomIB.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->DrawIndexed(Config::Room::INDEX_COUNT, 0, 0);

        m_context->RSSetState(nullptr);
    }

    if (m_stageMesh) {
        // White Material
        MaterialBuffer mbMat = {};
        mbMat.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        mbMat.specParams = { Config::Materials::STAGE_SPECULAR, Config::Materials::STAGE_SHININESS, 0.0f, 0.0f };
        m_materialBuffer.Update(m_context.Get(), mbMat);

        // Update world matrix for stage
        MatrixBuffer mbStage = mb;
        mbStage.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0.0f, m_stageOffset, 0.0f));
        m_matrixBuffer.Update(m_context.Get(), mbStage);

        m_stageMesh->Draw(m_context.Get());
    }

    // Restore main matrix buffer
    m_matrixBuffer.Update(m_context.Get(), mb);
    m_context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());

    if (firstFrame) Log("Stage Drawn");

    // Volumetric Pass - render to separate texture for blur
    m_volumetricData.jitter.x = m_time * Config::Volumetric::JITTER_SCALE;
    m_volumetricBuffer.Update(m_context.Get(), m_volumetricData);

    // Clear volumetric texture and render to it
    float blackColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_volRT.Clear(m_context.Get(), blackColor);
    m_volRT.Bind(m_context.Get());

    m_context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(1, 1, m_spotlightBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(2, 1, m_volumetricBuffer.GetAddressOf());

    ID3D11ShaderResourceView* volSRVs[] = { m_depthSRV.Get(), m_goboTexture ? m_goboTexture->GetSRV() : nullptr, m_shadowSRV.Get() };
    m_context->PSSetShaderResources(0, 3, volSRVs);

    ID3D11SamplerState* volSamplers[] = { m_samplerState.Get(), m_shadowSampler.Get() };
    m_context->PSSetSamplers(0, 2, volSamplers);

    m_volumetricShader.Bind(m_context.Get());
    UINT strideFS = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offsetFS = 0;
    m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(6, 0);

    m_context->PSSetShaderResources(0, 3, nullSRVs);

    if (firstFrame) Log("Volumetric Draw Done");

    // Blur volumetric texture
    if (m_enableVolBlur) {
        BlurBuffer bb;
        bb.texelSize = { 1.0f / Config::Display::WINDOW_WIDTH, 1.0f / Config::Display::WINDOW_HEIGHT };

        for (int pass = 0; pass < m_volBlurPasses; ++pass) {
            // Horizontal blur: volRTV -> blurTempRTV
            bb.direction = { 1.0f, 0.0f };
            m_blurBuffer.Update(m_context.Get(), bb);

            m_blurTempRT.Bind(m_context.Get());
            m_context->PSSetConstantBuffers(0, 1, m_blurBuffer.GetAddressOf());
            m_context->PSSetShaderResources(0, 1, m_volRT.GetSRVAddressOf());
            m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

            m_blurShader.Bind(m_context.Get());
            m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_context->Draw(6, 0);

            m_context->PSSetShaderResources(0, 1, nullSRVs);

            // Vertical blur: blurTempRTV -> volRTV
            bb.direction = { 0.0f, 1.0f };
            m_blurBuffer.Update(m_context.Get(), bb);

            m_volRT.Bind(m_context.Get());
            m_context->PSSetShaderResources(0, 1, m_blurTempRT.GetSRVAddressOf());

            m_blurShader.Bind(m_context.Get());
            m_context->Draw(6, 0);

            m_context->PSSetShaderResources(0, 1, nullSRVs);
        }

        if (firstFrame) Log("Volumetric Blur Done");
    }

    // Composite: add blurred volumetric to scene
    m_sceneRT.Bind(m_context.Get());
    m_context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);
    m_context->PSSetShaderResources(0, 1, m_volRT.GetSRVAddressOf());
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    m_compositeShader.Bind(m_context.Get());
    m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(6, 0);

    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_context->PSSetShaderResources(0, 1, nullSRVs);

    // FXAA Pass - render from scene texture to swap chain
    if (m_enableFXAA) {
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

        FXAABuffer fb;
        fb.rcpFrame = { 1.0f / Config::Display::WINDOW_WIDTH, 1.0f / Config::Display::WINDOW_HEIGHT };
        fb.padding = { 0.0f, 0.0f };
        m_fxaaBuffer.Update(m_context.Get(), fb);

        m_context->VSSetConstantBuffers(0, 1, m_fxaaBuffer.GetAddressOf());
        m_context->PSSetConstantBuffers(0, 1, m_fxaaBuffer.GetAddressOf());
        m_context->PSSetShaderResources(0, 1, m_sceneRT.GetSRVAddressOf());
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

        m_fxaaShader.Bind(m_context.Get());
        m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(6, 0);

        // Unbind scene SRV
        m_context->PSSetShaderResources(0, 1, nullSRVs);

        if (firstFrame) Log("FXAA Pass Done");
    } else {
        // No FXAA - just copy scene to swap chain
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

        m_context->PSSetShaderResources(0, 1, m_sceneRT.GetSRVAddressOf());
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

        m_compositeShader.Bind(m_context.Get());
        m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(6, 0);

        m_context->PSSetShaderResources(0, 1, nullSRVs);
    }
}

void Renderer::RenderUI() {
    static bool firstFrame = true;
    if (firstFrame) Log("RenderUI Started");

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
        ImGui::DragFloat("Step Count", &m_volumetricData.params.x, 1.0f, Config::Volumetric::MIN_STEP_COUNT, Config::Volumetric::MAX_STEP_COUNT);
        ImGui::SliderFloat("Density", &m_volumetricData.params.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Light Intensity Multiplier", &m_volumetricData.params.z, 0.0f, Config::Volumetric::DEFAULT_INTENSITY);
        ImGui::SliderFloat("Anisotropy (G)", &m_volumetricData.params.w, Config::Volumetric::MIN_ANISOTROPY, Config::Volumetric::MAX_ANISOTROPY);
    }

    if (ImGui::CollapsingHeader("Post Processing")) {
        ImGui::Checkbox("Enable FXAA", &m_enableFXAA);
        ImGui::Checkbox("Enable Volumetric Blur", &m_enableVolBlur);
        ImGui::SliderInt("Blur Passes", &m_volBlurPasses, Config::PostProcess::MIN_BLUR_PASSES, Config::PostProcess::MAX_BLUR_PASSES);
    }

    ImGui::End();

    if (firstFrame) {
        Log("RenderUI Done");
        firstFrame = false;
    }
}

void Renderer::EndFrame() {
    static bool firstFrame = true;
    if (firstFrame) Log("EndFrame Started");

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_graphics.Present(true);

    if (firstFrame) {
        Log("First EndFrame Completed Successfully!");
        firstFrame = false;
    }

    // Set all firstFrame statics to false after first success (handled locally in each function above)
    // Actually, I need to communicate this or just let it happen.
    // I'll add a member if I want it globally, but local statics are fine for breadcrumbs.
}
