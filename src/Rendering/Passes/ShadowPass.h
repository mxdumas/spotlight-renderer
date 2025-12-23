#pragma once

#include "IRenderPass.h"
#include "../../Resources/Shader.h"
#include "../../Core/ConstantBuffer.h"
#include "../../Core/Config.h"
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class Mesh;
struct SpotlightData;

// Matrix buffer for shadow pass (same layout as main pass for shader compatibility)
__declspec(align(16)) struct ShadowMatrixBuffer {
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX invViewProj;
    DirectX::XMFLOAT4 cameraPos;
};

// Shadow mapping pass - renders scene depth from spotlight's perspective
class ShadowPass : public IRenderPass {
public:
    ShadowPass() = default;
    ~ShadowPass() override = default;

    bool Initialize(ID3D11Device* device) override;
    void Shutdown() override;

    // Execute shadow map rendering
    // - spotData: spotlight position/direction/range for light matrix calculation
    // - mesh: the mesh to render into shadow map
    // - stageOffset: Y offset for mesh placement
    void Execute(ID3D11DeviceContext* context,
                 const SpotlightData& spotData,
                 Mesh* mesh,
                 float stageOffset);

    // Accessors for shadow resources used by other passes
    ID3D11ShaderResourceView* GetShadowSRV() const { return m_shadowSRV.Get(); }
    ID3D11SamplerState* GetShadowSampler() const { return m_shadowSampler.Get(); }

    // Get the light view-projection matrix for the last rendered frame
    DirectX::XMMATRIX GetLightViewProjection() const { return m_lightViewProj; }

private:
    // Shadow map resources
    ComPtr<ID3D11Texture2D> m_shadowMap;
    ComPtr<ID3D11DepthStencilView> m_shadowDSV;
    ComPtr<ID3D11ShaderResourceView> m_shadowSRV;
    ComPtr<ID3D11SamplerState> m_shadowSampler;

    // Shader and constant buffer
    Shader m_shadowShader;
    ConstantBuffer<ShadowMatrixBuffer> m_matrixBuffer;

    // Cached light view-projection matrix
    DirectX::XMMATRIX m_lightViewProj;
};
