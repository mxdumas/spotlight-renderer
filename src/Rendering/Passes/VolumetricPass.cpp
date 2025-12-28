#include "VolumetricPass.h"
#include "../RenderTarget.h"

bool VolumetricPass::Initialize(ID3D11Device *device)
{
    // Load volumetric shader with position-only layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> fsLayout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (!m_volumetricShader.LoadVertexShader(device, L"shaders/volumetric.hlsl", "VS", fsLayout))
        return false;
    if (!m_volumetricShader.LoadPixelShader(device, L"shaders/volumetric.hlsl", "PS"))
        return false;

    // Initialize constant buffer
    if (!m_volumetricBuffer.Initialize(device))
        return false;

    // Initialize spotlight array buffer
    if (!m_spotlightArrayBuffer.Initialize(device))
        return false;

    // Set default parameters
    m_params.params = {Config::Volumetric::DEFAULT_STEP_COUNT, Config::Volumetric::DEFAULT_DENSITY,
                       Config::Volumetric::DEFAULT_INTENSITY, Config::Volumetric::DEFAULT_ANISOTROPY};
    m_params.jitter = {0.0f, 0.0f, 0.0f, 0.0f};

    return true;
}

void VolumetricPass::Shutdown()
{
    // Shader cleans up automatically via ComPtr
}

void VolumetricPass::Execute(ID3D11DeviceContext *context, const std::vector<Spotlight> &spotlights,
                             RenderTarget *volumetricRt, ID3D11Buffer *fullScreenVb, ID3D11ShaderResourceView *depthSrv,
                             ID3D11ShaderResourceView *goboSrv, ID3D11ShaderResourceView *shadowSrv,
                             ID3D11SamplerState *sampler, ID3D11SamplerState *shadowSampler, float time)
{
    // Update jitter time
    m_params.jitter.x = time * Config::Volumetric::JITTER_SCALE;
    m_volumetricBuffer.Update(context, m_params);

    // Update spotlight buffer
    SpotlightArrayBuffer spotData;
    std::memset(&spotData, 0, sizeof(spotData)); // Clear potentially unused slots

    size_t count = (std::min)(spotlights.size(), static_cast<size_t>(Config::Spotlight::MAX_SPOTLIGHTS));
    for (size_t i = 0; i < count; ++i)
    {
        spotData.lights[i] = spotlights[i].GetGPUData();
    }
    m_spotlightArrayBuffer.Update(context, spotData);

    // Clear and bind volumetric render target
    float blackColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    volumetricRt->Clear(context, blackColor);
    volumetricRt->Bind(context);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(Config::Display::WINDOW_WIDTH);
    viewport.Height = static_cast<float>(Config::Display::WINDOW_HEIGHT);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // Bind constant buffers
    ID3D11Buffer *buffers[] = {m_spotlightArrayBuffer.Get(), m_volumetricBuffer.Get()};
    context->PSSetConstantBuffers(1, 2, buffers); // Start at slot 1 (SpotlightBuffer)

    // Bind textures: depth, gobo, shadow
    ID3D11ShaderResourceView *srvs[] = {depthSrv, goboSrv, shadowSrv};
    context->PSSetShaderResources(0, 3, srvs);

    // Bind samplers
    ID3D11SamplerState *samplers[] = {sampler, shadowSampler};
    context->PSSetSamplers(0, 2, samplers);

    // Bind shader and draw fullscreen quad
    m_volumetricShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &fullScreenVb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Unbind SRVs to avoid conflicts
    ID3D11ShaderResourceView *nullSrvs[3] = {nullptr};
    context->PSSetShaderResources(0, 3, nullSrvs);
}
