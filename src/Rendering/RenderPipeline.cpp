#include "RenderPipeline.h"
#include "../Geometry/GeometryGenerator.h"
#include "../Mesh.h"
#include "../Texture.h"

RenderPipeline::~RenderPipeline() {
    Shutdown();
}

bool RenderPipeline::Initialize(ID3D11Device* device) {
    m_device = device;

    // Create render passes
    m_shadowPass = std::make_unique<ShadowPass>();
    m_scenePass = std::make_unique<ScenePass>();
    m_volumetricPass = std::make_unique<VolumetricPass>();
    m_blurPass = std::make_unique<BlurPass>();
    m_compositePass = std::make_unique<CompositePass>();
    m_fxaaPass = std::make_unique<FXAAPass>();

    // Initialize all passes
    if (!m_shadowPass->Initialize(device)) return false;
    if (!m_scenePass->Initialize(device)) return false;
    if (!m_volumetricPass->Initialize(device)) return false;
    if (!m_blurPass->Initialize(device)) return false;
    if (!m_compositePass->Initialize(device)) return false;
    if (!m_fxaaPass->Initialize(device)) return false;

    // Create shared render targets
    if (!m_sceneRT.Create(device, Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT)) return false;
    if (!m_volRT.Create(device, Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT)) return false;
    if (!m_blurTempRT.Create(device, Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT)) return false;

    // Create fullscreen quad
    if (!GeometryGenerator::CreateFullScreenQuad(device, m_fullScreenVB)) return false;

    // Create linear sampler for general use
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

    HRESULT hr = device->CreateSamplerState(&sampDesc, &m_linearSampler);
    if (FAILED(hr)) return false;

    // Initialize constant buffers
    if (!m_matrixBuffer.Initialize(device)) return false;
    if (!m_spotlightBuffer.Initialize(device)) return false;
    if (!m_ceilingLightsBuffer.Initialize(device)) return false;

    // Set scene render target for scene pass
    m_scenePass->SetRenderTarget(&m_sceneRT);

    // Initialize volumetric params with defaults
    m_volumetricPass->GetParams().params = {
        Config::Volumetric::DEFAULT_STEP_COUNT,
        Config::Volumetric::DEFAULT_DENSITY,
        Config::Volumetric::DEFAULT_INTENSITY,
        Config::Volumetric::DEFAULT_ANISOTROPY
    };
    m_volumetricPass->GetParams().jitter = { 0.0f, 0.0f, 0.0f, 0.0f };

    return true;
}

void RenderPipeline::Shutdown() {
    if (m_shadowPass) m_shadowPass->Shutdown();
    if (m_scenePass) m_scenePass->Shutdown();
    if (m_volumetricPass) m_volumetricPass->Shutdown();
    if (m_blurPass) m_blurPass->Shutdown();
    if (m_compositePass) m_compositePass->Shutdown();
    if (m_fxaaPass) m_fxaaPass->Shutdown();

    m_sceneRT.Shutdown();
    m_volRT.Shutdown();
    m_blurTempRT.Shutdown();
    m_fullScreenVB.Reset();
    m_linearSampler.Reset();
}

void RenderPipeline::SetupViewport(ID3D11DeviceContext* context, int width, int height) {
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
}

void RenderPipeline::ClearShaderResources(ID3D11DeviceContext* context) {
    ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
    context->PSSetShaderResources(0, 8, nullSRVs);
}

void RenderPipeline::Render(ID3D11DeviceContext* context, const RenderContext& ctx) {
    // Clear all SRVs to prevent D3D warnings
    ClearShaderResources(context);

    // Update spotlight data
    ctx.spotlight->UpdateLightMatrix();
    ctx.spotlight->UpdateGoboShake(ctx.time);

    // 1. Shadow Pass
    RenderShadowPass(context, ctx);

    // 2. Scene Pass (renders to m_sceneRT)
    RenderScenePass(context, ctx);

    // 3. Volumetric Pass (renders to m_volRT)
    RenderVolumetricPass(context, ctx);

    // 4. Blur Pass (blurs m_volRT using m_blurTempRT as temp)
    if (m_enableVolBlur) {
        RenderBlurPass(context);
    }

    // 5. Composite Pass (adds volumetric to scene)
    RenderCompositePass(context);

    // 6. Final Pass (FXAA or direct copy to back buffer)
    RenderFinalPass(context, ctx);
}

void RenderPipeline::RenderShadowPass(ID3D11DeviceContext* context, const RenderContext& ctx) {
    m_shadowPass->Execute(context,
                          ctx.spotlight->GetGPUData(),
                          ctx.stageMesh,
                          ctx.stageOffset);
}

void RenderPipeline::RenderScenePass(ID3D11DeviceContext* context, const RenderContext& ctx) {
    // Bind scene render target with depth
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_sceneRT.Bind(context, ctx.depthStencilView);
    m_sceneRT.Clear(context, clearColor);
    context->ClearDepthStencilView(ctx.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    SetupViewport(context, Config::Display::WINDOW_WIDTH, Config::Display::WINDOW_HEIGHT);

    // Calculate camera matrices
    DirectX::XMMATRIX view = ctx.camera->GetViewMatrix();
    DirectX::XMMATRIX proj = ctx.camera->GetProjectionMatrix();

    // Update matrix buffer for identity world matrix (room)
    PipelineMatrixBuffer mb;
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    mb.view = DirectX::XMMatrixTranspose(view);
    mb.projection = DirectX::XMMatrixTranspose(proj);

    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, view);
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(nullptr, proj);
    mb.invViewProj = DirectX::XMMatrixTranspose(invProj * invView);

    mb.cameraPos = { ctx.cameraPos.x, ctx.cameraPos.y, ctx.cameraPos.z, 1.0f };

    m_matrixBuffer.Update(context, mb);
    m_spotlightBuffer.Update(context, ctx.spotlight->GetGPUData());

    // Update ceiling lights
    ctx.ceilingLights->Update();
    m_ceilingLightsBuffer.Update(context, ctx.ceilingLights->GetGPUData());

    // Bind constant buffers
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, m_spotlightBuffer.GetAddressOf());
    context->PSSetConstantBuffers(3, 1, m_ceilingLightsBuffer.GetAddressOf());

    // Bind textures
    ID3D11ShaderResourceView* goboSRV = ctx.goboTexture ? ctx.goboTexture->GetSRV() : nullptr;
    ID3D11ShaderResourceView* srvs[] = { goboSRV, m_shadowPass->GetShadowSRV() };
    context->PSSetShaderResources(0, 2, srvs);

    ID3D11SamplerState* samplers[] = { m_linearSampler.Get(), m_shadowPass->GetShadowSampler() };
    context->PSSetSamplers(0, 2, samplers);

    // Execute scene pass (room with identity world)
    m_scenePass->Execute(context,
                         ctx.depthStencilView,
                         ctx.roomVB,
                         ctx.roomIB,
                         nullptr, // Skip stage in scene pass for now
                         ctx.stageOffset,
                         ctx.roomSpecular,
                         ctx.roomShininess);

    // Render stage with offset world matrix
    if (ctx.stageMesh) {
        mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0.0f, ctx.stageOffset, 0.0f));
        m_matrixBuffer.Update(context, mb);

        MaterialBuffer mbMat = {};
        mbMat.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        mbMat.specParams = { Config::Materials::STAGE_SPECULAR, Config::Materials::STAGE_SHININESS, 0.0f, 0.0f };
        m_scenePass->GetMaterialBuffer().Update(context, mbMat);
        context->PSSetConstantBuffers(2, 1, m_scenePass->GetMaterialBuffer().GetAddressOf());

        m_scenePass->GetShader().Bind(context);
        ctx.stageMesh->Draw(context);
    }

    // Restore identity matrix
    mb.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    m_matrixBuffer.Update(context, mb);
}

void RenderPipeline::RenderVolumetricPass(ID3D11DeviceContext* context, const RenderContext& ctx) {
    // Update jitter time
    m_volumetricPass->GetParams().jitter.x = ctx.time * Config::Volumetric::JITTER_SCALE;

    // Bind matrix and spotlight buffers for volumetric pass
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, m_spotlightBuffer.GetAddressOf());

    ID3D11ShaderResourceView* goboSRV = ctx.goboTexture ? ctx.goboTexture->GetSRV() : nullptr;

    m_volumetricPass->Execute(context,
                              &m_volRT,
                              m_fullScreenVB.Get(),
                              ctx.depthSRV,
                              goboSRV,
                              m_shadowPass->GetShadowSRV(),
                              m_linearSampler.Get(),
                              m_shadowPass->GetShadowSampler(),
                              ctx.time);

    ClearShaderResources(context);
}

void RenderPipeline::RenderBlurPass(ID3D11DeviceContext* context) {
    m_blurPass->Execute(context,
                        &m_volRT,
                        &m_blurTempRT,
                        m_fullScreenVB.Get(),
                        m_linearSampler.Get(),
                        m_blurPasses);

    ClearShaderResources(context);
}

void RenderPipeline::RenderCompositePass(ID3D11DeviceContext* context) {
    m_compositePass->ExecuteAdditive(context,
                                     &m_sceneRT,
                                     &m_volRT,
                                     m_fullScreenVB.Get(),
                                     m_linearSampler.Get());

    ClearShaderResources(context);
}

void RenderPipeline::RenderFinalPass(ID3D11DeviceContext* context, const RenderContext& ctx) {
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    if (m_enableFXAA) {
        m_fxaaPass->Execute(context,
                           ctx.backBufferRTV,
                           &m_sceneRT,
                           m_fullScreenVB.Get(),
                           m_linearSampler.Get());
    } else {
        // Direct copy
        m_compositePass->ExecuteCopy(context,
                                     ctx.backBufferRTV,
                                     m_sceneRT.GetSRV(),
                                     m_fullScreenVB.Get(),
                                     m_linearSampler.Get());
    }

    ClearShaderResources(context);
}
