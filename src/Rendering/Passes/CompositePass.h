#pragma once

#include "IRenderPass.h"
#include "../../Resources/Shader.h"
#include "../../Core/Config.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class RenderTarget;

/**
 * @class CompositePass
 * @brief Blends the volumetric lighting effect with the scene rendering.
 * 
 * This pass uses additive blending to combine the accumulated volumetric light
 * from the volumetric pass into the main scene render target.
 */
class CompositePass : public IRenderPass {
public:
    /**
     * @brief Default constructor for the CompositePass class.
     */
    CompositePass() = default;

    /**
     * @brief Destructor for the CompositePass class.
     */
    ~CompositePass() override = default;

    /**
     * @brief Initializes the composite shader and blend state objects.
     * 
     * @param device Pointer to the ID3D11Device.
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize(ID3D11Device* device) override;

    /**
     * @brief Shuts down the pass and releases resources.
     */
    void Shutdown() override;

    /**
     * @brief Executes an additive composite of the volumetric lighting onto the scene.
     * 
     * @param context Pointer to the ID3D11DeviceContext.
     * @param sceneRT The scene render target to composite onto.
     * @param volumetricRT The volumetric texture containing the light effect.
     * @param fullScreenVB Vertex buffer for a full-screen quad.
     * @param sampler Sampler state for texture sampling.
     */
    void ExecuteAdditive(ID3D11DeviceContext* context,
                         RenderTarget* sceneRT,
                         RenderTarget* volumetricRT,
                         ID3D11Buffer* fullScreenVB,
                         ID3D11SamplerState* sampler);

    /**
     * @brief Executes a simple copy from a source texture to a destination render target.
     * 
     * Used typically as a fallback or when certain post-processing steps are disabled.
     * 
     * @param context Pointer to the ID3D11DeviceContext.
     * @param destRTV The destination render target view.
     * @param sourceSRV The source texture's shader resource view.
     * @param fullScreenVB Vertex buffer for a full-screen quad.
     * @param sampler Sampler state for texture sampling.
     */
    void ExecuteCopy(ID3D11DeviceContext* context,
                     ID3D11RenderTargetView* destRTV,
                     ID3D11ShaderResourceView* sourceSRV,
                     ID3D11Buffer* fullScreenVB,
                     ID3D11SamplerState* sampler);

private:
    Shader m_compositeShader;
    ComPtr<ID3D11BlendState> m_additiveBlendState;
};
