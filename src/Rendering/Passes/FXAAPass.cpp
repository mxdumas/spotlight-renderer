#include "FXAAPass.h"
#include "../RenderTarget.h"

bool FXAAPass::Initialize(ID3D11Device *device)
{
    // Load FXAA shader with position-only layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> fs_layout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (!m_fxaaShader.LoadVertexShader(device, L"shaders/fxaa.hlsl", "VS", fs_layout))
        return false;
    if (!m_fxaaShader.LoadPixelShader(device, L"shaders/fxaa.hlsl", "PS"))
        return false;

    // Initialize constant buffer
    if (!m_fxaaBuffer.Initialize(device))
        return false;

    return true;
}

void FXAAPass::Shutdown()
{
    // Shader cleans up automatically via ComPtr
}

void FXAAPass::Execute(ID3D11DeviceContext *context, ID3D11RenderTargetView *dest_rtv, RenderTarget *scene_rt,
                       ID3D11Buffer *full_screen_vb, ID3D11SamplerState *sampler)
{
    // Clear and bind destination (swap chain back buffer)
    float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    context->OMSetRenderTargets(1, &dest_rtv, nullptr);
    context->ClearRenderTargetView(dest_rtv, clear_color);

    // Update FXAA constant buffer
    FXAABuffer fb;
    fb.rcpFrame = {1.0f / Config::Display::WINDOW_WIDTH, 1.0f / Config::Display::WINDOW_HEIGHT};
    fb.padding = {0.0f, 0.0f};
    m_fxaaBuffer.Update(context, fb);

    // Bind constant buffers to both VS and PS
    context->VSSetConstantBuffers(0, 1, m_fxaaBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_fxaaBuffer.GetAddressOf());

    // Bind scene texture
    context->PSSetShaderResources(0, 1, scene_rt->GetSRVAddressOf());
    context->PSSetSamplers(0, 1, &sampler);

    // Draw fullscreen quad
    m_fxaaShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &full_screen_vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Unbind SRV
    ID3D11ShaderResourceView *null_srv = nullptr;
    context->PSSetShaderResources(0, 1, &null_srv);
}
