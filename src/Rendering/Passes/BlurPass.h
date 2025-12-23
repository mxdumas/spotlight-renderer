#pragma once

#include "IRenderPass.h"
#include "../../Shader.h"
#include "../../ConstantBuffer.h"
#include "../../Core/Config.h"
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

class RenderTarget;

// Blur parameters
__declspec(align(16)) struct BlurBuffer {
    DirectX::XMFLOAT2 texelSize;
    DirectX::XMFLOAT2 direction; // (1,0) for horizontal, (0,1) for vertical
};

// Gaussian blur pass - separable two-pass blur
class BlurPass : public IRenderPass {
public:
    BlurPass() = default;
    ~BlurPass() override = default;

    bool Initialize(ID3D11Device* device) override;
    void Shutdown() override;

    // Execute blur on a render target
    // - sourceRT: input texture (will also receive final output)
    // - tempRT: temporary buffer for ping-pong
    // - fullScreenVB: fullscreen quad vertex buffer
    // - sampler: linear sampler
    // - passes: number of blur iterations
    void Execute(ID3D11DeviceContext* context,
                 RenderTarget* sourceRT,
                 RenderTarget* tempRT,
                 ID3D11Buffer* fullScreenVB,
                 ID3D11SamplerState* sampler,
                 int passes);

private:
    Shader m_blurShader;
    ConstantBuffer<BlurBuffer> m_blurBuffer;
};
