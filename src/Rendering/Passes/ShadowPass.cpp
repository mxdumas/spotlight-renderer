#include "ShadowPass.h"
#include "../../Resources/Mesh.h"
#include "../../Scene/Spotlight.h"

bool ShadowPass::Initialize(ID3D11Device *device)
{
    // Create shadow map texture array (one slice per spotlight)
    D3D11_TEXTURE2D_DESC sm_desc = {};
    sm_desc.Width = Config::Shadow::MAP_SIZE;
    sm_desc.Height = Config::Shadow::MAP_SIZE;
    sm_desc.MipLevels = 1;
    sm_desc.ArraySize = Config::Spotlight::MAX_SPOTLIGHTS;
    sm_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    sm_desc.SampleDesc.Count = 1;
    sm_desc.Usage = D3D11_USAGE_DEFAULT;
    sm_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&sm_desc, nullptr, &m_shadowMap);
    if (FAILED(hr))
        return false;

    // Create depth stencil view for each array slice
    for (int i = 0; i < Config::Spotlight::MAX_SPOTLIGHTS; ++i)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv_desc.Texture2DArray.MipSlice = 0;
        dsv_desc.Texture2DArray.FirstArraySlice = i;
        dsv_desc.Texture2DArray.ArraySize = 1;
        hr = device->CreateDepthStencilView(m_shadowMap.Get(), &dsv_desc, &m_shadowDSV[i]);
        if (FAILED(hr))
            return false;
    }

    // Create shader resource view for the entire array
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Texture2DArray.MostDetailedMip = 0;
    srv_desc.Texture2DArray.MipLevels = 1;
    srv_desc.Texture2DArray.FirstArraySlice = 0;
    srv_desc.Texture2DArray.ArraySize = Config::Spotlight::MAX_SPOTLIGHTS;
    hr = device->CreateShaderResourceView(m_shadowMap.Get(), &srv_desc, &m_shadowSRV);
    if (FAILED(hr))
        return false;

    // Create shadow comparison sampler
    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samp_desc.BorderColor[0] = 1.0f;
    samp_desc.BorderColor[1] = 1.0f;
    samp_desc.BorderColor[2] = 1.0f;
    samp_desc.BorderColor[3] = 1.0f;
    samp_desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    hr = device->CreateSamplerState(&samp_desc, &m_shadowSampler);
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

    return true;
}

void ShadowPass::Shutdown()
{
    m_shadowSRV.Reset();
    for (int i = 0; i < Config::Spotlight::MAX_SPOTLIGHTS; ++i)
        m_shadowDSV[i].Reset();
    m_shadowMap.Reset();
    m_shadowSampler.Reset();
}

void ShadowPass::Execute(ID3D11DeviceContext *context, const SpotlightData &spot_data, int light_index, Mesh *mesh,
                         float stage_offset)
{
    if (!mesh || light_index < 0 || light_index >= Config::Spotlight::MAX_SPOTLIGHTS)
        return;

    // Clear shadow map depth for this light's slice
    context->ClearDepthStencilView(m_shadowDSV[light_index].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    context->OMSetRenderTargets(0, nullptr, m_shadowDSV[light_index].Get());

    // Set shadow map viewport
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(Config::Shadow::MAP_SIZE);
    vp.Height = static_cast<float>(Config::Shadow::MAP_SIZE);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context->RSSetViewports(1, &vp);

    // Use the pre-computed lightViewProj from SpotlightData to ensure consistency
    // between shadow map rendering and shadow sampling in shaders.
    // The matrix is stored transposed in SpotlightData, so we transpose it back.
    DirectX::XMMATRIX light_view_proj = DirectX::XMMatrixTranspose(spot_data.lightViewProj);

    // Update matrix buffer with combined light view-projection matrix
    // The shader uses viewProj directly instead of separate view/projection
    ShadowMatrixBuffer mb;
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0.0f, stage_offset, 0.0f));
    mb.view = DirectX::XMMatrixTranspose(light_view_proj); // viewProj in shader
    mb.projection = DirectX::XMMatrixIdentity();           // padding (unused)
    mb.invViewProj = DirectX::XMMatrixIdentity();          // padding (unused)
    mb.cameraPos = {0.0f, 0.0f, 0.0f, 0.0f};
    m_matrixBuffer.Update(context, mb);

    // Bind constant buffer and shader
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    m_shadowShader.Bind(context);

    // Draw mesh
    mesh->Draw(context);
}
