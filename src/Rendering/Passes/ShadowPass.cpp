#include "ShadowPass.h"
#include <cmath>
#include "../../Resources/Mesh.h"
#include "../../Scene/Spotlight.h"

bool ShadowPass::Initialize(ID3D11Device *device)
{
    // Create shadow map texture
    D3D11_TEXTURE2D_DESC smDesc = {};
    smDesc.Width = Config::Shadow::MAP_SIZE;
    smDesc.Height = Config::Shadow::MAP_SIZE;
    smDesc.MipLevels = 1;
    smDesc.ArraySize = 1;
    smDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    smDesc.SampleDesc.Count = 1;
    smDesc.Usage = D3D11_USAGE_DEFAULT;
    smDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&smDesc, nullptr, &m_shadowMap);
    if (FAILED(hr))
        return false;

    // Create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = device->CreateDepthStencilView(m_shadowMap.Get(), &dsvDesc, &m_shadowDSV);
    if (FAILED(hr))
        return false;

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(m_shadowMap.Get(), &srvDesc, &m_shadowSRV);
    if (FAILED(hr))
        return false;

    // Create shadow comparison sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampDesc.BorderColor[0] = 1.0f;
    sampDesc.BorderColor[1] = 1.0f;
    sampDesc.BorderColor[2] = 1.0f;
    sampDesc.BorderColor[3] = 1.0f;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    hr = device->CreateSamplerState(&sampDesc, &m_shadowSampler);
    if (FAILED(hr))
        return false;

    // Load shadow shader
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (!m_shadowShader.LoadVertexShader(device, L"shaders/shadow.hlsl", "VS", layout))
        return false;
    if (!m_shadowShader.LoadPixelShader(device, L"shaders/shadow.hlsl", "PS"))
        return false;

    // Initialize constant buffer
    if (!m_matrixBuffer.Initialize(device))
        return false;

    m_lightViewProj = DirectX::XMMatrixIdentity();

    return true;
}

void ShadowPass::Shutdown()
{
    m_shadowSRV.Reset();
    m_shadowDSV.Reset();
    m_shadowMap.Reset();
    m_shadowSampler.Reset();
}

void ShadowPass::Execute(ID3D11DeviceContext *context, const SpotlightData &spotData, Mesh *mesh, float stageOffset)
{
    if (!mesh)
        return;

    // Clear shadow map depth
    context->ClearDepthStencilView(m_shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    context->OMSetRenderTargets(0, nullptr, m_shadowDSV.Get());

    // Set shadow map viewport
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(Config::Shadow::MAP_SIZE);
    vp.Height = static_cast<float>(Config::Shadow::MAP_SIZE);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context->RSSetViewports(1, &vp);

    // Calculate light view/projection matrices
    DirectX::XMVECTOR lPos = DirectX::XMVectorSet(spotData.posRange.x, spotData.posRange.y, spotData.posRange.z, 1.0f);
    DirectX::XMVECTOR lDir = DirectX::XMVector3Normalize(
        DirectX::XMVectorSet(spotData.dirAngle.x, spotData.dirAngle.y, spotData.dirAngle.z, 0.0f));
    DirectX::XMVECTOR lUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // Handle near-vertical directions
    if (fabsf(DirectX::XMVectorGetY(lDir)) > 0.99f)
    {
        lUp = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }

    DirectX::XMMATRIX lView = DirectX::XMMatrixLookToLH(lPos, lDir, lUp);
    DirectX::XMMATRIX lProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1.0f, 0.1f, spotData.posRange.w);

    m_lightViewProj = lView * lProj;

    // Update matrix buffer with light matrices
    ShadowMatrixBuffer mb;
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0.0f, stageOffset, 0.0f));
    mb.view = DirectX::XMMatrixTranspose(lView);
    mb.projection = DirectX::XMMatrixTranspose(lProj);
    mb.invViewProj = DirectX::XMMatrixIdentity();
    mb.cameraPos = {0.0f, 0.0f, 0.0f, 0.0f};
    m_matrixBuffer.Update(context, mb);

    // Bind constant buffer and shader
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_shadowShader.Bind(context);

    // Draw mesh
    mesh->Draw(context);
}
