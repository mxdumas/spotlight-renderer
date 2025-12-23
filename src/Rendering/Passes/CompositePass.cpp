#include "CompositePass.h"
#include "../RenderTarget.h"

bool CompositePass::Initialize(ID3D11Device* device) {
    // Load composite shader with position-only layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> fsLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (!m_compositeShader.LoadVertexShader(device, L"shaders/composite.hlsl", "VS", fsLayout)) return false;
    if (!m_compositeShader.LoadPixelShader(device, L"shaders/composite.hlsl", "PS")) return false;

    // Create additive blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = device->CreateBlendState(&blendDesc, &m_additiveBlendState);
    if (FAILED(hr)) return false;

    return true;
}

void CompositePass::Shutdown() {
    m_additiveBlendState.Reset();
}

void CompositePass::ExecuteAdditive(ID3D11DeviceContext* context,
                                    RenderTarget* sceneRT,
                                    RenderTarget* volumetricRT,
                                    ID3D11Buffer* fullScreenVB,
                                    ID3D11SamplerState* sampler) {
    // Bind scene render target
    sceneRT->Bind(context);

    // Enable additive blending
    context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);

    // Bind volumetric texture
    context->PSSetShaderResources(0, 1, volumetricRT->GetSRVAddressOf());
    context->PSSetSamplers(0, 1, &sampler);

    // Draw fullscreen quad
    m_compositeShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &fullScreenVB, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Restore default blend state
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    // Unbind SRV
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
}

void CompositePass::ExecuteCopy(ID3D11DeviceContext* context,
                                ID3D11RenderTargetView* destRTV,
                                ID3D11ShaderResourceView* sourceSRV,
                                ID3D11Buffer* fullScreenVB,
                                ID3D11SamplerState* sampler) {
    // Clear and bind destination
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->OMSetRenderTargets(1, &destRTV, nullptr);
    context->ClearRenderTargetView(destRTV, clearColor);

    // Bind source texture
    context->PSSetShaderResources(0, 1, &sourceSRV);
    context->PSSetSamplers(0, 1, &sampler);

    // Draw fullscreen quad
    m_compositeShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &fullScreenVB, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Unbind SRV
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
}
