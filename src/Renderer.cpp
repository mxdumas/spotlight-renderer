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
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 1920;
    sd.BufferDesc.Height = 1080;
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
    depthDesc.Width = 1920;
    depthDesc.Height = 1080;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    hr = m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, &m_depthStencilView);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc = {};
    depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depthSRVDesc.Texture2D.MipLevels = 1;
    hr = m_device->CreateShaderResourceView(m_depthStencilBuffer.Get(), &depthSRVDesc, &m_depthSRV);
    if (FAILED(hr)) return false;

    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    D3D11_VIEWPORT viewport = {};
    viewport.Width = 1920.0f;
    viewport.Height = 1080.0f;
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

    float stageMinY = m_stageMesh->GetMinY();
    float floorY = -0.05f;
    m_stageOffset = floorY - stageMinY;

    m_fixturePos = { 0.0f, 15.0f, 0.0f };
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
    smDesc.Width = SHADOW_MAP_SIZE;
    smDesc.Height = SHADOW_MAP_SIZE;
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
    hr = m_device->CreateBuffer(&vbd, &vinit, &m_debugBoxVB);
    if (FAILED(hr)) { Log("Failed to create debugBoxVB: " + std::to_string(hr)); return false; }

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = { indices };
    hr = m_device->CreateBuffer(&ibd, &iinit, &m_debugBoxIB);
    if (FAILED(hr)) { Log("Failed to create debugBoxIB: " + std::to_string(hr)); return false; }
    Log("Debug Buffers Created");

    // Create Spotlight Cone Proxy
    {
        std::vector<float> coneVertices;
        std::vector<uint32_t> coneIndices;

        // Tip
        coneVertices.push_back(0.0f); coneVertices.push_back(0.0f); coneVertices.push_back(0.0f);

        const int segments = 16;
        const float radius = 1.0f;
        const float height = 1.0f;

        for (int i = 0; i < segments; ++i) {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            coneVertices.push_back(cosf(angle) * radius);
            coneVertices.push_back(sinf(angle) * radius);
            coneVertices.push_back(height);
        }

        for (int i = 0; i < segments; ++i) {
            // Lines from tip
            coneIndices.push_back(0);
            coneIndices.push_back(i + 1);

            // Lines for base circle
            coneIndices.push_back(i + 1);
            coneIndices.push_back(((i + 1) % segments) + 1);
        }

        D3D11_BUFFER_DESC cvbd = {};
        cvbd.Usage = D3D11_USAGE_DEFAULT;
        cvbd.ByteWidth = (UINT)(coneVertices.size() * sizeof(float));
        cvbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA cvinit = { coneVertices.data() };
        hr = m_device->CreateBuffer(&cvbd, &cvinit, &m_coneVB);
        if (FAILED(hr)) { Log("Failed to create coneVB: " + std::to_string(hr)); return false; }

        D3D11_BUFFER_DESC cibd = {};
        cibd.Usage = D3D11_USAGE_DEFAULT;
        cibd.ByteWidth = (UINT)(coneIndices.size() * sizeof(uint32_t));
        cibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ciinit = { coneIndices.data() };
        hr = m_device->CreateBuffer(&cibd, &ciinit, &m_coneIB);
        if (FAILED(hr)) { Log("Failed to create coneIB: " + std::to_string(hr)); return false; }
        m_coneIndexCount = (UINT)coneIndices.size();
        Log("Cone Proxy Created");
    }

    // Create Room Cube (100x100 width, floor at -0.05)
    {
        float r = 50.0f;
        float floorY = -0.05f;
        float ceilY = 100.0f;
        
        float roomVerts[] = {
            // Back (-Z)
            -r, floorY, -r,  0,0, 1,  0,1, // Inverted normals to face inward
             r, floorY, -r,  0,0, 1,  1,1,
             r, ceilY,  -r,  0,0, 1,  1,0,
            -r, ceilY,  -r,  0,0, 1,  0,0,
            // Front (+Z)
            -r, floorY,  r,  0,0,-1,  0,1,
             r, floorY,  r,  0,0,-1,  1,1,
             r, ceilY,   r,  0,0,-1,  1,0,
            -r, ceilY,   r,  0,0,-1,  0,0,
            // Left (-X)
            -r, floorY,  r,  1,0,0,   0,1,
            -r, floorY, -r,  1,0,0,   1,1,
            -r, ceilY,  -r,  1,0,0,   1,0,
            -r, ceilY,   r,  1,0,0,   0,0,
            // Right (+X)
             r, floorY,  r, -1,0,0,   0,1,
             r, floorY, -r, -1,0,0,   1,1,
             r, ceilY,  -r, -1,0,0,   1,0,
             r, ceilY,   r, -1,0,0,   0,0,
            // Bottom (-Y)
            -r, floorY,  r,  0,1,0,  0,1,
            -r, floorY, -r,  0,1,0,  1,1,
             r, floorY, -r,  0,1,0,  1,0,
             r, floorY,  r,  0,1,0,  0,0,
            // Top (+Y)
            -r, ceilY,   r,  0,-1,0,   0,1,
            -r, ceilY,  -r,  0,-1,0,   1,1,
             r, ceilY,  -r,  0,-1,0,   1,0,
             r, ceilY,   r,  0,-1,0,   0,0,
        };

        // Standard CCW indices for inward facing normals
        uint32_t roomInds[] = {
            // Bottom
            16,17,18, 16,18,19,
            // Top
            20,22,21, 20,23,22,
            // Back
            0,1,2, 0,2,3,
            // Front
            4,6,5, 4,7,6,
            // Left
            8,9,10, 8,10,11,
            // Right
            12,14,13, 12,15,14
        };

        D3D11_BUFFER_DESC rvbd = {};
        rvbd.Usage = D3D11_USAGE_DEFAULT;
        rvbd.ByteWidth = sizeof(roomVerts);
        rvbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA rvinit = { roomVerts };
        hr = m_device->CreateBuffer(&rvbd, &rvinit, &m_roomVB);
        if (FAILED(hr)) { Log("Failed to create roomVB: " + std::to_string(hr)); return false; }

        D3D11_BUFFER_DESC ribd = {};
        ribd.Usage = D3D11_USAGE_DEFAULT;
        ribd.ByteWidth = sizeof(roomInds);
        ribd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA riinit = { roomInds };
        hr = m_device->CreateBuffer(&ribd, &riinit, &m_roomIB);
        if (FAILED(hr)) { Log("Failed to create roomIB: " + std::to_string(hr)); return false; }
        // Store index count if needed or just use 6
        Log("Room Cube Created");
    }

    // Create Debug Sphere
    {
        std::vector<float> verts;
        std::vector<uint32_t> inds;
        const int stacks = 10;
        const int slices = 10;
        const float radius = 2.0f;

        for (int i = 0; i <= stacks; ++i) {
            float lat = (float)i / stacks * 3.14159f;
            float y = cosf(lat) * radius;
            float r = sinf(lat) * radius;
            for (int j = 0; j <= slices; ++j) {
                float lon = (float)j / slices * 2.0f * 3.14159f;
                float x = cosf(lon) * r;
                float z = sinf(lon) * r;
                
                // Pos
                verts.push_back(x); verts.push_back(y); verts.push_back(z);
                // Normal (same as pos/radius)
                verts.push_back(x/radius); verts.push_back(y/radius); verts.push_back(z/radius);
                // UV
                verts.push_back((float)j/slices); verts.push_back((float)i/stacks);
            }
        }

        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int first = (i * (slices + 1)) + j;
                int second = first + slices + 1;
                inds.push_back(first);
                inds.push_back(second);
                inds.push_back(first + 1);

                inds.push_back(second);
                inds.push_back(second + 1);
                inds.push_back(first + 1);
            }
        }

        D3D11_BUFFER_DESC svbd = {};
        svbd.Usage = D3D11_USAGE_DEFAULT;
        svbd.ByteWidth = (UINT)(verts.size() * sizeof(float));
        svbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA svinit = { verts.data() };
        hr = m_device->CreateBuffer(&svbd, &svinit, &m_sphereVB);
        if (FAILED(hr)) { Log("Failed to create sphereVB: " + std::to_string(hr)); return false; }

        D3D11_BUFFER_DESC sibd = {};
        sibd.Usage = D3D11_USAGE_DEFAULT;
        sibd.ByteWidth = (UINT)(inds.size() * sizeof(uint32_t));
        sibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA siinit = { inds.data() };
        hr = m_device->CreateBuffer(&sibd, &siinit, &m_sphereIB);
        if (FAILED(hr)) { Log("Failed to create sphereIB: " + std::to_string(hr)); return false; }
        m_sphereIndexCount = (UINT)inds.size();
        Log("Debug Sphere Created");
    }

    // Create full screen quad
    float fsVertices[] = {
        -1.0f, -1.0f, 0.0f,  -1.0f,  1.0f, 0.0f,   1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  -1.0f,  1.0f, 0.0f,   1.0f,  1.0f, 0.0f
    };
    D3D11_BUFFER_DESC fsVbd = {};
    fsVbd.Usage = D3D11_USAGE_DEFAULT;
    fsVbd.ByteWidth = sizeof(fsVertices);
    fsVbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA fsVinit = { fsVertices };
    hr = m_device->CreateBuffer(&fsVbd, &fsVinit, &m_fullScreenVB);
    if (FAILED(hr)) { Log("Failed to create fullScreenVB: " + std::to_string(hr)); return false; }
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

    // Initial spotlight data
    memset(&m_spotlightData, 0, sizeof(m_spotlightData));
    m_spotlightData.posRange = { m_fixturePos.x, m_fixturePos.y, m_fixturePos.z, 500.0f };
    
    // Point towards center of stage
    DirectX::XMVECTOR lPosVec = DirectX::XMLoadFloat3(&m_fixturePos);
    DirectX::XMVECTOR lDirVec = DirectX::XMVector3Normalize(DirectX::XMVectorNegate(lPosVec));
    DirectX::XMStoreFloat4(&m_spotlightData.dirAngle, lDirVec);

    m_spotlightData.colorInt = { 1.0f, 1.0f, 1.0f, 100.0f };
    m_spotlightData.coneGobo = { 0.98f, 0.71f, 0.0f, 0.0f };
    m_spotlightData.goboOff = { 0.0f, 0.0f, 0.0f, 0.0f };

    m_volumetricData.params = { 512.0f, 0.2f, 10.0f, 0.5f };
    m_volumetricData.jitter = { 0.0f, 0.0f, 0.0f, 0.0f };

    m_time = 0.0f;
    m_goboShakeAmount = 0.0f;
    m_useCMY = false;
    m_cmy = { 0.0f, 0.0f, 0.0f };
    m_ceilingLightIntensity = 40.0f;
    m_ambientFill = 2.0f;
    m_roomSpecular = 0.8f;
    m_roomShininess = 64.0f;

    // Initialize Camera
    m_camDistance = 40.0f;
    m_camPitch = 0.4f;
    m_camYaw = 0.0f;
    m_camTarget = { 0.0f, 0.0f, 0.0f };
    m_camera.SetPerspective(DirectX::XM_PIDIV4, 1920.0f / 1080.0f, 0.1f, 1000.0f);

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

    // Create scene render target for FXAA
    D3D11_TEXTURE2D_DESC sceneDesc = {};
    sceneDesc.Width = 1920;
    sceneDesc.Height = 1080;
    sceneDesc.MipLevels = 1;
    sceneDesc.ArraySize = 1;
    sceneDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sceneDesc.SampleDesc.Count = 1;
    sceneDesc.SampleDesc.Quality = 0;
    sceneDesc.Usage = D3D11_USAGE_DEFAULT;
    sceneDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = m_device->CreateTexture2D(&sceneDesc, nullptr, &m_sceneTexture);
    if (FAILED(hr)) { Log("Failed to create scene texture"); return false; }

    hr = m_device->CreateRenderTargetView(m_sceneTexture.Get(), nullptr, &m_sceneRTV);
    if (FAILED(hr)) { Log("Failed to create scene RTV"); return false; }

    hr = m_device->CreateShaderResourceView(m_sceneTexture.Get(), nullptr, &m_sceneSRV);
    if (FAILED(hr)) { Log("Failed to create scene SRV"); return false; }

    // Create volumetric render target (for separate blur)
    hr = m_device->CreateTexture2D(&sceneDesc, nullptr, &m_volTexture);
    if (FAILED(hr)) { Log("Failed to create vol texture"); return false; }
    hr = m_device->CreateRenderTargetView(m_volTexture.Get(), nullptr, &m_volRTV);
    if (FAILED(hr)) { Log("Failed to create vol RTV"); return false; }
    hr = m_device->CreateShaderResourceView(m_volTexture.Get(), nullptr, &m_volSRV);
    if (FAILED(hr)) { Log("Failed to create vol SRV"); return false; }

    // Create blur temp texture
    hr = m_device->CreateTexture2D(&sceneDesc, nullptr, &m_blurTempTexture);
    if (FAILED(hr)) { Log("Failed to create blur temp texture"); return false; }
    hr = m_device->CreateRenderTargetView(m_blurTempTexture.Get(), nullptr, &m_blurTempRTV);
    if (FAILED(hr)) { Log("Failed to create blur temp RTV"); return false; }
    hr = m_device->CreateShaderResourceView(m_blurTempTexture.Get(), nullptr, &m_blurTempSRV);
    if (FAILED(hr)) { Log("Failed to create blur temp SRV"); return false; }

    if (!m_blurBuffer.Initialize(m_device.Get())) {
        Log("Failed to initialize blur constant buffer");
        return false;
    }

    m_enableFXAA = true;
    m_enableVolBlur = true;
    m_volBlurPasses = 1;
    m_ceilingLightIntensity = 1.0f; // Default 1.0

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

    m_shadowSRV.Reset();
    m_shadowDSV.Reset();
    m_shadowMap.Reset();
    m_shadowSampler.Reset();

    m_depthSRV.Reset();
    m_additiveBlendState.Reset();

    m_fullScreenVB.Reset();
    m_debugBoxIB.Reset();
    m_debugBoxVB.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
    m_stageMesh.reset();
    m_goboTexture.reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}

void Renderer::RenderShadowMap() {
    m_context->ClearDepthStencilView(m_shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_context->OMSetRenderTargets(0, nullptr, m_shadowDSV.Get());

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)SHADOW_MAP_SIZE;
    vp.Height = (float)SHADOW_MAP_SIZE;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    // The light matrices are already in m_spotlightData.lightViewProj (transposed)
    // We need untransposed for the MatrixBuffer used by shadow shader
    DirectX::XMVECTOR lPos = DirectX::XMVectorSet(m_spotlightData.posRange.x, m_spotlightData.posRange.y, m_spotlightData.posRange.z, 1.0f);
    DirectX::XMVECTOR lDir = DirectX::XMVector3Normalize(DirectX::XMVectorSet(m_spotlightData.dirAngle.x, m_spotlightData.dirAngle.y, m_spotlightData.dirAngle.z, 0.0f));
    DirectX::XMVECTOR lUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(DirectX::XMVectorGetY(lDir)) > 0.99f) lUp = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    
    DirectX::XMMATRIX lView = DirectX::XMMatrixLookToLH(lPos, lDir, lUp);
    DirectX::XMMATRIX lProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1.0f, 0.1f, m_spotlightData.posRange.w);

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
    m_time += 0.016f; // Approx 60fps for now
    
    // Unbind all SRVs to avoid conflicts with depth/stencil and render targets
    ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
    m_context->PSSetShaderResources(0, 8, nullSRVs);

    static bool firstFrame = true;
    if (firstFrame) Log("First BeginFrame Started");

    // Update spotlight matrices first
    DirectX::XMVECTOR lPos = DirectX::XMVectorSet(m_spotlightData.posRange.x, m_spotlightData.posRange.y, m_spotlightData.posRange.z, 1.0f);
    DirectX::XMVECTOR lDir = DirectX::XMVector3Normalize(DirectX::XMVectorSet(m_spotlightData.dirAngle.x, m_spotlightData.dirAngle.y, m_spotlightData.dirAngle.z, 0.0f));
    DirectX::XMVECTOR lUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(DirectX::XMVectorGetY(lDir)) > 0.99f) lUp = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    
    DirectX::XMMATRIX lView = DirectX::XMMatrixLookToLH(lPos, lDir, lUp);
    DirectX::XMMATRIX lProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1.0f, 0.1f, m_spotlightData.posRange.w);
    m_spotlightData.lightViewProj = DirectX::XMMatrixTranspose(lView * lProj);

    m_spotlightData.goboOff.x = sinf(m_time * 30.0f) * m_goboShakeAmount * 0.05f;
    m_spotlightData.goboOff.y = cosf(m_time * 35.0f) * m_goboShakeAmount * 0.05f;

    // Render Shadow Map
    RenderShadowMap();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    if (firstFrame) Log("ImGui NewFrame Done");

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // Render to scene texture (for FXAA), not directly to swap chain
    m_context->OMSetRenderTargets(1, m_sceneRTV.GetAddressOf(), m_depthStencilView.Get());
    m_context->ClearRenderTargetView(m_sceneRTV.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = 1920.0f;
    viewport.Height = 1080.0f;
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
    m_spotlightBuffer.Update(m_context.Get(), m_spotlightData);

    // Update Ceiling Lights
    CeilingLightsBuffer clb;
    // Grid 4 in X, 2 in Z.
    // X range: -40 to 40. Spacing = 80 / 3 = 26.
    // Z range: -20 to 20. Spacing = 40.
    
    int idx = 0;
    for (int z = 0; z < 2; ++z) {
        for (int x = 0; x < 4; ++x) {
            float posX = -40.0f + x * 26.6f;
            float posZ = -20.0f + z * 40.0f;
            clb.lights[idx].pos = { posX, 95.0f, posZ, 200.0f }; // Range 200
            clb.lights[idx].color = { 1.0f, 1.0f, 1.0f, m_ceilingLightIntensity * 500.0f };
            idx++;
        }
    }
    float ambVal = m_ambientFill / 100.0f; // 0-1 range
    clb.ambient = { ambVal, ambVal, ambVal, 1.0f };
    m_ceilingLightsBuffer.Update(m_context.Get(), clb);

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
        mb.color = { 0.2f, 0.2f, 0.2f, 1.0f };
        mb.specParams = { m_roomSpecular, m_roomShininess, 0.0f, 0.0f };
        m_materialBuffer.Update(m_context.Get(), mb);

        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        ComPtr<ID3D11RasterizerState> rs;
        m_device->CreateRasterizerState(&rd, &rs);
        m_context->RSSetState(rs.Get());

        UINT stride = 32;
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_roomVB.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(m_roomIB.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->DrawIndexed(36, 0, 0); // 6 faces * 2 tris * 3 verts

        m_context->RSSetState(nullptr);
    }

    if (m_stageMesh) {
        // White Material
        MaterialBuffer mbMat = {};
        mbMat.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        mbMat.specParams = { 0.1f, 16.0f, 0.0f, 0.0f }; // Subtle spec for stage
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
    m_volumetricData.jitter.x = m_time * 0.005f; // Slow down temporal jitter
    m_volumetricBuffer.Update(m_context.Get(), m_volumetricData);

    // Clear volumetric texture and render to it
    float blackColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context->ClearRenderTargetView(m_volRTV.Get(), blackColor);
    m_context->OMSetRenderTargets(1, m_volRTV.GetAddressOf(), nullptr);

    m_context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(1, 1, m_spotlightBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(2, 1, m_volumetricBuffer.GetAddressOf());

    ID3D11ShaderResourceView* volSRVs[] = { m_depthSRV.Get(), m_goboTexture ? m_goboTexture->GetSRV() : nullptr, m_shadowSRV.Get() };
    m_context->PSSetShaderResources(0, 3, volSRVs);

    ID3D11SamplerState* volSamplers[] = { m_samplerState.Get(), m_shadowSampler.Get() };
    m_context->PSSetSamplers(0, 2, volSamplers);

    m_volumetricShader.Bind(m_context.Get());
    UINT strideFS = 12;
    UINT offsetFS = 0;
    m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->Draw(6, 0);

    m_context->PSSetShaderResources(0, 3, nullSRVs);

    if (firstFrame) Log("Volumetric Draw Done");

    // Blur volumetric texture
    if (m_enableVolBlur) {
        BlurBuffer bb;
        bb.texelSize = { 1.0f / 1920.0f, 1.0f / 1080.0f };

        for (int pass = 0; pass < m_volBlurPasses; ++pass) {
            // Horizontal blur: volRTV -> blurTempRTV
            bb.direction = { 1.0f, 0.0f };
            m_blurBuffer.Update(m_context.Get(), bb);

            m_context->OMSetRenderTargets(1, m_blurTempRTV.GetAddressOf(), nullptr);
            m_context->PSSetConstantBuffers(0, 1, m_blurBuffer.GetAddressOf());
            m_context->PSSetShaderResources(0, 1, m_volSRV.GetAddressOf());
            m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

            m_blurShader.Bind(m_context.Get());
            m_context->IASetVertexBuffers(0, 1, m_fullScreenVB.GetAddressOf(), &strideFS, &offsetFS);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_context->Draw(6, 0);

            m_context->PSSetShaderResources(0, 1, nullSRVs);

            // Vertical blur: blurTempRTV -> volRTV
            bb.direction = { 0.0f, 1.0f };
            m_blurBuffer.Update(m_context.Get(), bb);

            m_context->OMSetRenderTargets(1, m_volRTV.GetAddressOf(), nullptr);
            m_context->PSSetShaderResources(0, 1, m_blurTempSRV.GetAddressOf());

            m_blurShader.Bind(m_context.Get());
            m_context->Draw(6, 0);

            m_context->PSSetShaderResources(0, 1, nullSRVs);
        }

        if (firstFrame) Log("Volumetric Blur Done");
    }

    // Composite: add blurred volumetric to scene
    m_context->OMSetRenderTargets(1, m_sceneRTV.GetAddressOf(), nullptr);
    m_context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);
    m_context->PSSetShaderResources(0, 1, m_volSRV.GetAddressOf());
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
        fb.rcpFrame = { 1.0f / 1920.0f, 1.0f / 1080.0f };
        fb.padding = { 0.0f, 0.0f };
        m_fxaaBuffer.Update(m_context.Get(), fb);

        m_context->VSSetConstantBuffers(0, 1, m_fxaaBuffer.GetAddressOf());
        m_context->PSSetConstantBuffers(0, 1, m_fxaaBuffer.GetAddressOf());
        m_context->PSSetShaderResources(0, 1, m_sceneSRV.GetAddressOf());
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

        m_context->PSSetShaderResources(0, 1, m_sceneSRV.GetAddressOf());
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
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
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
        ImGui::Text("Environment");
        ImGui::SliderFloat("Ceiling Light Intensity", &m_ceilingLightIntensity, 1.0f, 100.0f);
        ImGui::SliderFloat("Ambient Fill", &m_ambientFill, 0.0f, 100.0f);
        ImGui::SliderFloat("Room Specular", &m_roomSpecular, 0.0f, 1.0f);
        ImGui::SliderFloat("Room Shininess", &m_roomShininess, 1.0f, 128.0f);
        ImGui::Separator();

        ImGui::Text("Transform");
        ImGui::DragFloat3("Position", &m_spotlightData.posRange.x, 0.1f);
        ImGui::DragFloat3("Direction", &m_spotlightData.dirAngle.x, 0.01f);
        if (ImGui::Button("Reset to Fixture")) {
            m_spotlightData.posRange.x = m_fixturePos.x;
            m_spotlightData.posRange.y = m_fixturePos.y;
            m_spotlightData.posRange.z = m_fixturePos.z;
        }

        ImGui::Separator();
        ImGui::Text("Color & Intensity");
        ImGui::Checkbox("Use CMY Mixing", &m_useCMY);
        if (m_useCMY) {
            if (ImGui::ColorEdit3("CMY", &m_cmy.x)) {
                m_spotlightData.colorInt.x = 1.0f - m_cmy.x;
                m_spotlightData.colorInt.y = 1.0f - m_cmy.y;
                m_spotlightData.colorInt.z = 1.0f - m_cmy.z;
            }
        } else {
            ImGui::ColorEdit3("RGB Color", &m_spotlightData.colorInt.x);
        }
        ImGui::DragFloat("Intensity", &m_spotlightData.colorInt.w, 1.0f, 0.0f, 5000.0f);
        ImGui::DragFloat("Range", &m_spotlightData.posRange.w, 1.0f, 10.0f, 1000.0f);

        ImGui::Separator();
        ImGui::Text("Beam Shape");
        ImGui::SliderFloat("Beam Angle", &m_spotlightData.coneGobo.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Field Angle", &m_spotlightData.coneGobo.y, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Gobo Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Rotation", &m_spotlightData.coneGobo.z, 0.01f);
        ImGui::SliderFloat("Shake Amount", &m_goboShakeAmount, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Volumetric Quality")) {
        ImGui::DragFloat("Step Count", &m_volumetricData.params.x, 1.0f, 16.0f, 512.0f);
        ImGui::SliderFloat("Density", &m_volumetricData.params.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Light Intensity Multiplier", &m_volumetricData.params.z, 0.0f, 10.0f);
        ImGui::SliderFloat("Anisotropy (G)", &m_volumetricData.params.w, -0.99f, 0.99f);
    }

    if (ImGui::CollapsingHeader("Post Processing")) {
        ImGui::Checkbox("Enable FXAA", &m_enableFXAA);
        ImGui::Checkbox("Enable Volumetric Blur", &m_enableVolBlur);
        ImGui::SliderInt("Blur Passes", &m_volBlurPasses, 1, 5);
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
