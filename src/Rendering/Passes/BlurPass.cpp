#include "BlurPass.h"
#include "../RenderTarget.h"

bool BlurPass::Initialize(ID3D11Device *device)
{
    // Load blur shader with position-only layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> fs_layout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (!m_blurShader.LoadVertexShader(device, L"shaders/blur.hlsl", "VS", fs_layout))
        return false;
    if (!m_blurShader.LoadPixelShader(device, L"shaders/blur.hlsl", "PS"))
        return false;

    // Initialize constant buffer
    if (!m_blurBuffer.Initialize(device))
        return false;

    return true;
}

void BlurPass::Shutdown()
{
    // Shader cleans up automatically via ComPtr
}

void BlurPass::Execute(ID3D11DeviceContext *context, RenderTarget *source_rt, RenderTarget *temp_rt,
                       ID3D11Buffer *full_screen_vb, ID3D11SamplerState *sampler, int passes)
{
    if (passes <= 0)
        return;

    BlurBuffer bb;
    bb.texelSize = {1.0f / Config::Display::WINDOW_WIDTH, 1.0f / Config::Display::WINDOW_HEIGHT};

    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    ID3D11ShaderResourceView *null_srv = nullptr;

    m_blurShader.Bind(context);
    context->IASetVertexBuffers(0, 1, &full_screen_vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (int pass = 0; pass < passes; ++pass)
    {
        // Horizontal blur: sourceRT -> tempRT
        bb.direction = {1.0f, 0.0f};
        m_blurBuffer.Update(context, bb);

        temp_rt->Bind(context);
        context->PSSetConstantBuffers(0, 1, m_blurBuffer.GetAddressOf());
        context->PSSetShaderResources(0, 1, source_rt->GetSRVAddressOf());
        context->PSSetSamplers(0, 1, &sampler);
        context->Draw(6, 0);

        // Unbind SRV
        context->PSSetShaderResources(0, 1, &null_srv);

        // Vertical blur: temp_rt -> source_rt
        bb.direction = {0.0f, 1.0f};
        m_blurBuffer.Update(context, bb);

        source_rt->Bind(context);
        context->PSSetShaderResources(0, 1, temp_rt->GetSRVAddressOf());
        context->Draw(6, 0);

        // Unbind SRV
        context->PSSetShaderResources(0, 1, &null_srv);
    }
}
