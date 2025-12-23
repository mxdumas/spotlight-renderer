#pragma once

#include "IRenderPass.h"
#include "../../Shader.h"
#include "../../ConstantBuffer.h"
#include "../../Core/Config.h"
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

class RenderTarget;

// FXAA parameters
__declspec(align(16)) struct FXAABuffer {
    DirectX::XMFLOAT2 rcpFrame; // 1.0 / screenSize
    DirectX::XMFLOAT2 padding;
};

// FXAA anti-aliasing pass
class FXAAPass : public IRenderPass {
public:
    FXAAPass() = default;
    ~FXAAPass() override = default;

    bool Initialize(ID3D11Device* device) override;
    void Shutdown() override;

    // Execute FXAA
    // - destRTV: destination render target (typically swap chain back buffer)
    // - sceneRT: source scene texture
    // - fullScreenVB: fullscreen quad vertex buffer
    // - sampler: linear sampler
    void Execute(ID3D11DeviceContext* context,
                 ID3D11RenderTargetView* destRTV,
                 RenderTarget* sceneRT,
                 ID3D11Buffer* fullScreenVB,
                 ID3D11SamplerState* sampler);

private:
    Shader m_fxaaShader;
    ConstantBuffer<FXAABuffer> m_fxaaBuffer;
};
