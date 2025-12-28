#include "CompositePass.h"
#include "../RenderTarget.h"

bool CompositePass::Initialize(ID3D11Device *device)
{
    // Load composite shader with position-only layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> fs_layout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (!m_compositeShader.LoadVertexShader(device, L"shaders/composite.hlsl", "VS", fs_layout))
        return false;
    if (!m_compositeShader.LoadPixelShader(device, L"shaders/composite.hlsl", "PS"))
        return false;

    // Create additive blend state
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = device->CreateBlendState(&blend_desc, &m_additiveBlendState);
    return SUCCEEDED(hr);
}

void CompositePass::Shutdown()
{
    m_additiveBlendState.Reset();
}

void CompositePass::ExecuteAdditive(ID3D11DeviceContext *context, RenderTarget *scene_rt, RenderTarget *volumetric_rt,
                                    ID3D11Buffer *full_screen_vb, ID3D11SamplerState *sampler)
{
    // Bind scene render target
    scene_rt->Bind(context);

    // Enable additive blending
    context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);

    // Bind volumetric texture
    context->PSSetShaderResources(0, 1, volumetric_rt->GetSRVAddressOf());
    context->PSSetSamplers(0, 1, &sampler);

    // Draw fullscreen quad
    m_compositeShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &full_screen_vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Restore default blend state
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    // Unbind SRV
    ID3D11ShaderResourceView *null_srv = nullptr;
    context->PSSetShaderResources(0, 1, &null_srv);
}

void CompositePass::ExecuteCopy(ID3D11DeviceContext *context, ID3D11RenderTargetView *dest_rtv,
                                ID3D11ShaderResourceView *source_srv, ID3D11Buffer *full_screen_vb,
                                ID3D11SamplerState *sampler)
{
    // Clear and bind destination
    float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    context->OMSetRenderTargets(1, &dest_rtv, nullptr);
    context->ClearRenderTargetView(dest_rtv, clear_color);

    // Bind source texture
    context->PSSetShaderResources(0, 1, &source_srv);
    context->PSSetSamplers(0, 1, &sampler);

    // Draw fullscreen quad
    m_compositeShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &full_screen_vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Unbind SRV
    ID3D11ShaderResourceView *null_srv = nullptr;
    context->PSSetShaderResources(0, 1, &null_srv);
}
