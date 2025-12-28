#pragma once

#include <DirectXMath.h>
#include <vector>
#include <wrl/client.h>
#include "../../Core/Config.h"
#include "../../Core/ConstantBuffer.h"
#include "../../Resources/Shader.h"
#include "IRenderPass.h"

using Microsoft::WRL::ComPtr;

class Mesh;
struct SpotlightData;

/**
 * @struct ShadowMatrixBuffer
 * @brief Matrix buffer for the shadow pass.
 *
 * Uses the same layout as the main pipeline matrix buffer for shader compatibility.
 * The shader uses viewProj directly instead of separate view/projection matrices.
 */
__declspec(align(16)) struct ShadowMatrixBuffer
{
    DirectX::XMMATRIX world;     ///< World transformation matrix.
    DirectX::XMMATRIX viewProj;  ///< Combined light view-projection matrix.
    DirectX::XMMATRIX padding1;  ///< Unused, kept for layout alignment.
    DirectX::XMMATRIX padding2;  ///< Unused, kept for layout alignment.
    DirectX::XMFLOAT4 cameraPos; ///< Unused, kept for layout alignment.
};

/**
 * @class ShadowPass
 * @brief Renders the scene's depth from each spotlight's perspective into a shadow map array.
 *
 * The generated shadow map array is used in subsequent passes (scene and volumetric)
 * to calculate shadows and light occlusion for up to MAX_SPOTLIGHTS spotlights.
 */
class ShadowPass : public IRenderPass
{
public:
    /**
     * @brief Default constructor for the ShadowPass class.
     */
    ShadowPass() = default;

    /**
     * @brief Destructor for the ShadowPass class.
     */
    ~ShadowPass() override = default;

    /**
     * @brief Initializes shadow map textures, views, shaders, and samplers.
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
     * @brief Executes the shadow map rendering for a specific light.
     *
     * Renders the specified mesh into the shadow map array at the given light index.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param spot_data Parameters of the spotlight used for light matrix calculation.
     * @param light_index Index of the light (0 to MAX_SHADOW_LIGHTS-1).
     * @param mesh Pointer to the mesh to render (usually the stage).
     * @param stage_offset Vertical offset for the mesh.
     */
    void Execute(ID3D11DeviceContext *context, const SpotlightData &spot_data, int light_index, Mesh *mesh,
                 float stage_offset);

    /**
     * @brief Gets the shader resource view of the shadow map array.
     * @return Pointer to the shadow map array SRV.
     */
    [[nodiscard]] ID3D11ShaderResourceView *GetShadowSRV() const
    {
        return m_shadowSRV.Get();
    }

    /**
     * @brief Gets the sampler state used for shadow comparison.
     * @return Pointer to the shadow sampler state.
     */
    [[nodiscard]] ID3D11SamplerState *GetShadowSampler() const
    {
        return m_shadowSampler.Get();
    }

private:
    // Shadow map array resources
    ComPtr<ID3D11Texture2D> m_shadowMap;
    ComPtr<ID3D11DepthStencilView> m_shadowDSV[Config::Spotlight::MAX_SPOTLIGHTS];
    ComPtr<ID3D11ShaderResourceView> m_shadowSRV;
    ComPtr<ID3D11SamplerState> m_shadowSampler;

    // Shader and constant buffer
    Shader m_shadowShader;
    ConstantBuffer<ShadowMatrixBuffer> m_matrixBuffer;
};
