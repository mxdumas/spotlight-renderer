#pragma once

#include "RenderTarget.h"
#include "Passes/ShadowPass.h"
#include "Passes/ScenePass.h"
#include "Passes/VolumetricPass.h"
#include "Passes/BlurPass.h"
#include "Passes/CompositePass.h"
#include "Passes/FXAAPass.h"
#include "../ConstantBuffer.h"
#include "../Camera.h"
#include "../Scene/Spotlight.h"
#include "../Scene/CeilingLights.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class Mesh;
class Texture;

// Matrix buffer for main pass
__declspec(align(16)) struct PipelineMatrixBuffer {
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX invViewProj;
    DirectX::XMFLOAT4 cameraPos;
};

// Context data needed for rendering a frame
struct RenderContext {
    // Camera
    Camera* camera;
    DirectX::XMFLOAT3 cameraPos;

    // Scene data
    Spotlight* spotlight;
    CeilingLights* ceilingLights;
    Mesh* stageMesh;
    Texture* goboTexture;
    float stageOffset;
    float time;

    // Room geometry (owned by caller)
    ID3D11Buffer* roomVB;
    ID3D11Buffer* roomIB;

    // Room material
    float roomSpecular;
    float roomShininess;

    // Depth buffer for scene pass
    ID3D11DepthStencilView* depthStencilView;
    ID3D11ShaderResourceView* depthSRV;

    // Final output
    ID3D11RenderTargetView* backBufferRTV;
};

// Orchestrates all render passes in the correct order
class RenderPipeline {
public:
    RenderPipeline() = default;
    ~RenderPipeline();

    // Initialize all passes and shared resources
    bool Initialize(ID3D11Device* device);

    // Clean up all resources
    void Shutdown();

    // Execute full render pipeline
    void Render(ID3D11DeviceContext* context, const RenderContext& ctx);

    // Configuration
    void SetFXAAEnabled(bool enabled) { m_enableFXAA = enabled; }
    bool IsFXAAEnabled() const { return m_enableFXAA; }

    void SetVolumetricBlurEnabled(bool enabled) { m_enableVolBlur = enabled; }
    bool IsVolumetricBlurEnabled() const { return m_enableVolBlur; }

    void SetBlurPasses(int passes) { m_blurPasses = passes; }
    int GetBlurPasses() const { return m_blurPasses; }

    // Access to volumetric params for UI
    VolumetricBuffer& GetVolumetricParams() { return m_volumetricPass->GetParams(); }
    const VolumetricBuffer& GetVolumetricParams() const { return m_volumetricPass->GetParams(); }

private:
    void RenderShadowPass(ID3D11DeviceContext* context, const RenderContext& ctx);
    void RenderScenePass(ID3D11DeviceContext* context, const RenderContext& ctx);
    void RenderVolumetricPass(ID3D11DeviceContext* context, const RenderContext& ctx);
    void RenderBlurPass(ID3D11DeviceContext* context);
    void RenderCompositePass(ID3D11DeviceContext* context);
    void RenderFinalPass(ID3D11DeviceContext* context, const RenderContext& ctx);

    void SetupViewport(ID3D11DeviceContext* context, int width, int height);
    void ClearShaderResources(ID3D11DeviceContext* context);

    // Render passes
    std::unique_ptr<ShadowPass> m_shadowPass;
    std::unique_ptr<ScenePass> m_scenePass;
    std::unique_ptr<VolumetricPass> m_volumetricPass;
    std::unique_ptr<BlurPass> m_blurPass;
    std::unique_ptr<CompositePass> m_compositePass;
    std::unique_ptr<FXAAPass> m_fxaaPass;

    // Shared render targets
    RenderTarget m_sceneRT;
    RenderTarget m_volRT;
    RenderTarget m_blurTempRT;

    // Shared geometry
    ComPtr<ID3D11Buffer> m_fullScreenVB;

    // Shared samplers
    ComPtr<ID3D11SamplerState> m_linearSampler;

    // Constant buffers
    ConstantBuffer<PipelineMatrixBuffer> m_matrixBuffer;
    ConstantBuffer<SpotlightData> m_spotlightBuffer;
    ConstantBuffer<CeilingLightsData> m_ceilingLightsBuffer;

    // Configuration state
    bool m_enableFXAA = true;
    bool m_enableVolBlur = true;
    int m_blurPasses = Config::PostProcess::DEFAULT_BLUR_PASSES;

    // Cached device pointer (for sampler creation if needed)
    ID3D11Device* m_device = nullptr;
};
