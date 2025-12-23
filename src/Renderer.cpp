#include "Renderer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <iostream>
#include <fstream>

Renderer::Renderer() {}

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
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 1280;
    sd.BufferDesc.Height = 720;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    Log("Creating Device and SwapChain...");
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_context
    );

    if (FAILED(hr)) {
        Log("D3D11CreateDeviceAndSwapChain Failed with HR: " + std::to_string(hr));
        return false;
    }
    Log("Device & SwapChain Created Successfully");

    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
    if (FAILED(hr)) return false;

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr)) return false;

    // Create Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = 1280;
    depthDesc.Height = 720;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(hr)) return false;

    hr = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, &m_depthStencilView);
    if (FAILED(hr)) return false;

    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    D3D11_VIEWPORT viewport = {};
    viewport.Width = 1280.0f;
    viewport.Height = 720.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    m_context->RSSetViewports(1, &viewport);
    Log("Viewport and Render Targets Set");

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

    m_fixturePos = { 0.0f, 15.0f, 0.0f };
    for (const auto& shape : m_stageMesh->GetShapes()) {
        if (shape.name == "Cylinder.000") {
            m_fixturePos = shape.center;
            break;
        }
    }

    // Create debug cube
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    };
    uint32_t indices[] = {
        0,2,1, 0,3,2, 1,6,5, 1,2,6, 5,7,4, 5,6,7, 4,3,0, 4,7,3, 3,6,2, 3,7,6, 4,1,5, 4,0,1
    };

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = { vertices };
    m_device->CreateBuffer(&vbd, &vinit, &m_debugBoxVB);

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = { indices };
    m_device->CreateBuffer(&ibd, &iinit, &m_debugBoxIB);
    Log("Debug Buffers Created");

    // Initial spotlight data
    m_spotlightData.position = m_fixturePos;
    m_spotlightData.range = 50.0f;
    m_spotlightData.direction = { 0.0f, -1.0f, 0.0f };
    m_spotlightData.spotAngle = 0.5f;
    m_spotlightData.color = { 1.0f, 1.0f, 1.0f };
    m_spotlightData.intensity = 500.0f;
    m_spotlightData.angles = { 0.95f, 0.8f };

    // Initialize Camera
    m_camDistance = 40.0f;
    m_camPitch = 0.4f;
    m_camYaw = 0.0f;
    m_camTarget = { 0.0f, 0.0f, 0.0f };
    m_camera.SetPerspective(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);

    // Initialize Constant Buffers
    if (!m_matrixBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize matrix constant buffer");
        return false;
    }
    if (!m_spotlightBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize spotlight constant buffer");
        return false;
    }

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

    m_debugBoxIB.Reset();
    m_debugBoxVB.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
    m_stageMesh.reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}

void Renderer::BeginFrame() {
    static bool firstFrame = true;
    if (firstFrame) Log("First BeginFrame Started");

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    if (firstFrame) Log("ImGui NewFrame Done");

    float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    if (firstFrame) Log("Clear Views Done");

    // Update Camera
    float camX = m_camDistance * cosf(m_camPitch) * sinf(m_camYaw);
    float camY = m_camDistance * sinf(m_camPitch);
    float camZ = -m_camDistance * cosf(m_camPitch) * cosf(m_camYaw);
    m_camera.SetLookAt({ camX, camY, camZ }, m_camTarget, { 0.0f, 1.0f, 0.0f });

    // Update matrices
    MatrixBuffer mb;
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    mb.view = DirectX::XMMatrixTranspose(m_camera.GetViewMatrix());
    mb.projection = DirectX::XMMatrixTranspose(m_camera.GetProjectionMatrix());
    m_matrixBuffer.Update(m_context.Get(), mb);

    m_spotlightBuffer.Update(m_context.Get(), m_spotlightData);

    if (firstFrame) Log("Buffers Updated");

    m_context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(1, 1, m_spotlightBuffer.GetAddressOf());

    m_basicShader.Bind(m_context.Get());
    if (m_stageMesh) {
        m_stageMesh->Draw(m_context.Get());
    }

    if (firstFrame) Log("Stage Drawn");

    // Render debug box
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(m_fixturePos.x, m_fixturePos.y, m_fixturePos.z));
    m_matrixBuffer.Update(m_context.Get(), mb);
    
    m_debugShader.Bind(m_context.Get());
    UINT stride = 12;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, m_debugBoxVB.GetAddressOf(), &stride, &offset);
    m_context->IASetIndexBuffer(m_debugBoxIB.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->DrawIndexed(36, 0, 0);

    if (firstFrame) Log("Debug Box Drawn");
}

void Renderer::RenderUI() {
    static bool firstFrame = true;
    if (firstFrame) Log("RenderUI Started");
    
    ImGui::Begin("Camera Controls");
    ImGui::DragFloat("Distance", &m_camDistance, 0.1f, 1.0f, 200.0f);
    ImGui::SliderAngle("Pitch", &m_camPitch, -89.0f, 89.0f);
    ImGui::SliderAngle("Yaw", &m_camYaw, -180.0f, 180.0f);
    ImGui::DragFloat3("Target", &m_camTarget.x, 0.1f);
    ImGui::End();

    ImGui::Begin("Spotlight Controls");
    ImGui::DragFloat3("Position", &m_spotlightData.position.x, 0.1f);
    ImGui::DragFloat3("Direction", &m_spotlightData.direction.x, 0.01f);
    ImGui::ColorEdit3("Color", &m_spotlightData.color.x);
    ImGui::DragFloat("Intensity", &m_spotlightData.intensity, 1.0f, 0.0f, 2000.0f);
    ImGui::DragFloat("Beam Angle", &m_spotlightData.angles.x, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Field Angle", &m_spotlightData.angles.y, 0.01f, 0.0f, 1.0f);
    
    if (ImGui::Button("Reset to Fixture")) {
        m_spotlightData.position = m_fixturePos;
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
    m_swapChain->Present(1, 0);

    if (firstFrame) {
        Log("First EndFrame Completed Successfully!");
        firstFrame = false;
    }

    // Set all firstFrame statics to false after first success (handled locally in each function above)
    // Actually, I need to communicate this or just let it happen.
    // I'll add a member if I want it globally, but local statics are fine for breadcrumbs.
}
