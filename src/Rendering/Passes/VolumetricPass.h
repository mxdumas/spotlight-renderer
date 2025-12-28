#pragma once

#include <DirectXMath.h>
#include <vector>
#include <wrl/client.h>
#include "../../Core/Config.h"
#include "../../Core/ConstantBuffer.h"
#include "../../Resources/Shader.h"
#include "../../Scene/Spotlight.h"
#include "IRenderPass.h"

using Microsoft::WRL::ComPtr;

class RenderTarget;

/**
 * @struct VolumetricBuffer
 * @brief Parameters for the volumetric lighting (ray marching) shader.
 */
__declspec(align(16)) struct VolumetricBuffer
{
    DirectX::XMFLOAT4 params; ///< x: stepCount, y: density, z: intensity, w: anisotropy.
    DirectX::XMFLOAT4 jitter; ///< x: time-based jitter offset, yzw: unused.
};

/**
 * @struct SpotlightArrayBuffer
 * @brief Array of spotlights for the volumetric shader.
 */
__declspec(align(16)) struct SpotlightArrayBuffer
{
    SpotlightData lights[Config::Spotlight::MAX_SPOTLIGHTS];
};

/**
 * @class VolumetricPass
 * @brief Simulates light scattering through a volume using ray marching.
 *
 * This pass renders the spotlight's cone by sampling the shadow map and gobo texture
 * along rays from the camera, creating the "god rays" or "volumetric lighting" effect.
 */
class VolumetricPass : public IRenderPass
{
public:
    /**
     * @brief Default constructor for the VolumetricPass class.
     */
    VolumetricPass() = default;

    /**
     * @brief Destructor for the VolumetricPass class.
     */
    ~VolumetricPass() override = default;

    /**
     * @brief Initializes the volumetric shader and constant buffers.
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
     * @brief Executes the volumetric lighting rendering.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param spotlights List of active spotlights in the scene.
     * @param volumetricRt The render target where the volumetric effect will be rendered.
     * @param fullScreenVb Vertex buffer for a full-screen quad.
     * @param depthSrv Shader resource view of the scene's depth buffer.
     * @param goboSrv Shader resource view of the spotlight's gobo texture.
     * @param shadowSrv Shader resource view of the light's shadow map.
     * @param sampler Linear sampler for texture sampling.
     * @param shadowSampler Comparison sampler for shadow map sampling.
     * @param time Total elapsed time used for jittering.
     */
    void Execute(ID3D11DeviceContext *context, const std::vector<Spotlight> &spotlights, RenderTarget *volumetricRt,
                 ID3D11Buffer *fullScreenVb, ID3D11ShaderResourceView *depthSrv, ID3D11ShaderResourceView *goboSrv,
                 ID3D11ShaderResourceView *shadowSrv, ID3D11SamplerState *sampler, ID3D11SamplerState *shadowSampler,
                 float time);

    /**
     * @brief Gets a reference to the internal volumetric parameters.
     * @return Reference to the VolumetricBuffer.
     */
    VolumetricBuffer &GetParams()
    {
        return m_params;
    }

    /**
     * @brief Gets a const reference to the internal volumetric parameters.
     * @return Const reference to the VolumetricBuffer.
     */
    [[nodiscard]] const VolumetricBuffer &GetParams() const
    {
        return m_params;
    }

    /**
     * @brief Gets the constant buffer used for volumetric parameters.
     * @return Reference to the ConstantBuffer of VolumetricBuffer.
     */
    ConstantBuffer<VolumetricBuffer> &GetBuffer()
    {
        return m_volumetricBuffer;
    }

private:
    Shader m_volumetricShader;
    ConstantBuffer<VolumetricBuffer> m_volumetricBuffer;
    ConstantBuffer<SpotlightArrayBuffer> m_spotlightArrayBuffer;
    VolumetricBuffer m_params;
};
