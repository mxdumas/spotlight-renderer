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
 * @struct BlurBuffer
 * @brief Parameters for the Gaussian blur shader.
 */
__declspec(align(16)) struct BlurBuffer
{
    DirectX::XMFLOAT2 texelSize; ///< Size of a single texel in UV space (1/width, 1/height).
    DirectX::XMFLOAT2 direction; ///< Direction of the blur: (1,0) for horizontal, (0,1) for vertical.
};

/**
 * @class BlurPass
 * @brief Implements a separable Gaussian blur effect.
 *
 * This pass performs a two-pass blur (horizontal then vertical) to efficiently
 * blur the contents of a render target.
 */
class BlurPass : public IRenderPass
{
public:
    /**
     * @brief Default constructor for the BlurPass class.
     */
    BlurPass() = default;

    /**
     * @brief Destructor for the BlurPass class.
     */
    ~BlurPass() override = default;

    /**
     * @brief Initializes the blur shaders and constant buffers.
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
     * @brief Executes the Gaussian blur on a render target.
     *
     * This method performs multiple iterations of a two-pass blur using a ping-pong
     * technique between the source and temporary render targets.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param sourceRt The source render target (also receives the final result).
     * @param tempRt A temporary render target used for intermediate blur steps.
     * @param fullScreenVb Vertex buffer for a full-screen quad.
     * @param sampler Sampler state for texture sampling.
     * @param passes The number of blur iterations to perform.
     */
    void Execute(ID3D11DeviceContext *context, RenderTarget *sourceRt, RenderTarget *tempRt, ID3D11Buffer *fullScreenVb,
                 ID3D11SamplerState *sampler, int passes);

private:
    Shader m_blurShader;
    ConstantBuffer<BlurBuffer> m_blurBuffer;
};
