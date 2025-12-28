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
                             RenderTarget *volumetric_rt, ID3D11Buffer *full_screen_vb,
                             ID3D11ShaderResourceView *depth_srv, ID3D11ShaderResourceView *gobo_srv,
                             ID3D11ShaderResourceView *shadow_srv, ID3D11SamplerState *sampler,
                             ID3D11SamplerState *shadow_sampler, float time)
{
    // Update jitter time
    m_params.jitter.x = time * Config::Volumetric::JITTER_SCALE;
    m_volumetricBuffer.Update(context, m_params);

    // Update spotlight buffer
    SpotlightArrayBuffer spot_data;
    std::memset(&spot_data, 0, sizeof(spot_data)); // Clear potentially unused slots

    size_t count = (std::min)(spotlights.size(), static_cast<size_t>(Config::Spotlight::MAX_SPOTLIGHTS));
    for (size_t i = 0; i < count; ++i)
    {
        spot_data.lights[i] = spotlights[i].GetGPUData();
    }
    m_spotlightArrayBuffer.Update(context, spot_data);

    // Clear and bind volumetric render target
    float black_color[] = {0.0f, 0.0f, 0.0f, 0.0f};
    volumetric_rt->Clear(context, black_color);
    volumetric_rt->Bind(context);

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
    ID3D11ShaderResourceView *srvs[] = {depth_srv, gobo_srv, shadow_srv};
    context->PSSetShaderResources(0, 3, srvs);

    // Bind samplers
    ID3D11SamplerState *samplers[] = {sampler, shadow_sampler};
    context->PSSetSamplers(0, 2, samplers);

    // Bind shader and draw fullscreen quad
    m_volumetricShader.Bind(context);
    UINT stride = Config::Vertex::STRIDE_POSITION_ONLY;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &full_screen_vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0);

    // Unbind SRVs to avoid conflicts
    ID3D11ShaderResourceView *null_srvs[3] = {nullptr};
    context->PSSetShaderResources(0, 3, null_srvs);
}
