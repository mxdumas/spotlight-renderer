#pragma once

#include "IRenderPass.h"
#include "../../Shader.h"
#include "../../ConstantBuffer.h"
#include "../../Core/Config.h"
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

class RenderTarget;

// Volumetric lighting parameters
__declspec(align(16)) struct VolumetricBuffer {
    DirectX::XMFLOAT4 params; // x: stepCount, y: density, z: intensity, w: anisotropy
    DirectX::XMFLOAT4 jitter; // x: time, yzw: unused
};

// Volumetric lighting pass - ray marches through spotlight cone
class VolumetricPass : public IRenderPass {
public:
    VolumetricPass() = default;
    ~VolumetricPass() override = default;

    bool Initialize(ID3D11Device* device) override;
    void Shutdown() override;

    // Execute volumetric rendering
    // - volumetricRT: render target to output volumetric effect
    // - fullScreenVB: fullscreen quad vertex buffer
    // - depthSRV: scene depth buffer as SRV
    // - goboSRV: gobo texture
    // - shadowSRV: shadow map
    // - sampler: linear sampler
    // - shadowSampler: shadow comparison sampler
    // - time: animation time for jitter
    void Execute(ID3D11DeviceContext* context,
                 RenderTarget* volumetricRT,
                 ID3D11Buffer* fullScreenVB,
                 ID3D11ShaderResourceView* depthSRV,
                 ID3D11ShaderResourceView* goboSRV,
                 ID3D11ShaderResourceView* shadowSRV,
                 ID3D11SamplerState* sampler,
                 ID3D11SamplerState* shadowSampler,
                 float time);

    // Get/Set volumetric parameters
    VolumetricBuffer& GetParams() { return m_params; }
    const VolumetricBuffer& GetParams() const { return m_params; }

    // Access constant buffer for external binding
    ConstantBuffer<VolumetricBuffer>& GetBuffer() { return m_volumetricBuffer; }

private:
    Shader m_volumetricShader;
    ConstantBuffer<VolumetricBuffer> m_volumetricBuffer;
    VolumetricBuffer m_params;
};
