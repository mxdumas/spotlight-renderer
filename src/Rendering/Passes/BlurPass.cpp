#include "BlurPass.h"
#include "../RenderTarget.h"

bool BlurPass::Initialize(ID3D11Device *device)
{
    // Load blur shader with position-only layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> fsLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (!m_blurShader.LoadVertexShader(device, L"shaders/blur.hlsl", "VS", fsLayout))
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

void BlurPass::Execute(ID3D11DeviceContext *context, RenderTarget *sourceRt, RenderTarget *tempRt,
                       ID3D11Buffer *fullScreenVb, ID3D11SamplerState *sampler, int passes)
{
    if (passes <= 0)
        return;

    BlurBuffer bb;
    bb.texelSize = {1.0f / Config::Display::WINDOW_WIDTH, 1.0f / Config::Display::WINDOW_HEIGHT};

    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    ID3D11ShaderResourceView *nullSrv = nullptr;

    m_blurShader.Bind(context);
    context->IASetVertexBuffers(0, 1, &fullScreenVb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (int pass = 0; pass < passes; ++pass)
    {
        // Horizontal blur: sourceRT -> tempRT
        bb.direction = {1.0f, 0.0f};
        m_blurBuffer.Update(context, bb);

        tempRt->Bind(context);
        context->PSSetConstantBuffers(0, 1, m_blurBuffer.GetAddressOf());
        context->PSSetShaderResources(0, 1, sourceRt->GetSRVAddressOf());
        context->PSSetSamplers(0, 1, &sampler);
        context->Draw(6, 0);

        // Unbind SRV
        context->PSSetShaderResources(0, 1, &nullSrv);

        // Vertical blur: temp_rt -> source_rt
        bb.direction = {0.0f, 1.0f};
        m_blurBuffer.Update(context, bb);

        sourceRt->Bind(context);
        context->PSSetShaderResources(0, 1, tempRt->GetSRVAddressOf());
        context->Draw(6, 0);

        // Unbind SRV
        context->PSSetShaderResources(0, 1, &nullSrv);
    }
}
