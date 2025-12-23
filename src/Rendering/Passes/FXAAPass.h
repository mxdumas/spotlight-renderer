#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include "../../Core/Config.h"
#include "../../Core/ConstantBuffer.h"
#include "../../Resources/Shader.h"
#include "IRenderPass.h"

using Microsoft::WRL::ComPtr;

class RenderTarget;

/**
 * @struct FXAABuffer
 * @brief Parameters for the FXAA (Fast Approximate Anti-Aliasing) shader.
 */
__declspec(align(16)) struct FXAABuffer
{
    DirectX::XMFLOAT2 rcpFrame; ///< Reciprocal of the screen dimensions (1/width, 1/height).
    DirectX::XMFLOAT2 padding;  ///< Padding for 16-byte alignment.
};

/**
 * @class FXAAPass
 * @brief Implements the FXAA anti-aliasing post-processing effect.
 *
 * FXAA is a screen-space anti-aliasing technique that smooths edges in the final image.
 */
class FXAAPass : public IRenderPass
{
public:
    /**
     * @brief Default constructor for the FXAAPass class.
     */
    FXAAPass() = default;

    /**
     * @brief Destructor for the FXAAPass class.
     */
    ~FXAAPass() override = default;

    /**
     * @brief Initializes the FXAA shader and constant buffer.
     *
     * @param device Pointer to the ID3D11Device.
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize(ID3D11Device *device) override;

    /**
     * @brief Shuts down the pass and releases resources.
     */
    void Shutdown() override;

    /**
     * @brief Executes the FXAA anti-aliasing pass.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param destRTV The destination render target view (usually the back buffer).
     * @param sceneRT The source scene render target to apply FXAA to.
     * @param fullScreenVB Vertex buffer for a full-screen quad.
     * @param sampler Sampler state for texture sampling.
     */
    void Execute(ID3D11DeviceContext *context, ID3D11RenderTargetView *destRTV, RenderTarget *sceneRT,
                 ID3D11Buffer *fullScreenVB, ID3D11SamplerState *sampler);

private:
    Shader m_fxaaShader;
    ConstantBuffer<FXAABuffer> m_fxaaBuffer;
};
