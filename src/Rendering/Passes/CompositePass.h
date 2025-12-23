#pragma once

#include "IRenderPass.h"
#include "../../Resources/Shader.h"
#include "../../Core/Config.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class RenderTarget;

// Composite pass - blends volumetric effect onto scene
class CompositePass : public IRenderPass {
public:
    CompositePass() = default;
    ~CompositePass() override = default;

    bool Initialize(ID3D11Device* device) override;
    void Shutdown() override;

    // Execute additive composite of volumetric onto scene
    // - sceneRT: scene render target to composite onto
    // - volumetricRT: volumetric texture to add
    // - fullScreenVB: fullscreen quad vertex buffer
    // - sampler: linear sampler
    void ExecuteAdditive(ID3D11DeviceContext* context,
                         RenderTarget* sceneRT,
                         RenderTarget* volumetricRT,
                         ID3D11Buffer* fullScreenVB,
                         ID3D11SamplerState* sampler);

    // Execute simple copy (for non-FXAA path)
    // - destRTV: destination render target view
    // - sourceSRV: source texture SRV
    // - fullScreenVB: fullscreen quad vertex buffer
    // - sampler: linear sampler
    void ExecuteCopy(ID3D11DeviceContext* context,
                     ID3D11RenderTargetView* destRTV,
                     ID3D11ShaderResourceView* sourceSRV,
                     ID3D11Buffer* fullScreenVB,
                     ID3D11SamplerState* sampler);

private:
    Shader m_compositeShader;
    ComPtr<ID3D11BlendState> m_additiveBlendState;
};
