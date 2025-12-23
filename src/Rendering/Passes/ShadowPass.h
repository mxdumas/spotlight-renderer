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

/**
 * @struct ShadowMatrixBuffer
 * @brief Matrix buffer for the shadow pass.
 * 
 * Uses the same layout as the main pipeline matrix buffer for shader compatibility.
 */
__declspec(align(16)) struct ShadowMatrixBuffer {
    DirectX::XMMATRIX world;        ///< World transformation matrix.
    DirectX::XMMATRIX view;         ///< View transformation matrix (light's perspective).
    DirectX::XMMATRIX projection;   ///< Projection transformation matrix (light's perspective).
    DirectX::XMMATRIX invViewProj;  ///< Inverse view-projection matrix.
    DirectX::XMFLOAT4 cameraPos;    ///< Not used in shadow pass, but kept for layout alignment.
};

/**
 * @class ShadowPass
 * @brief Renders the scene's depth from the spotlight's perspective into a shadow map.
 * 
 * The generated shadow map is used in subsequent passes (scene and volumetric)
 * to calculate shadows and light occlusion.
 */
class ShadowPass : public IRenderPass {
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
    bool Initialize(ID3D11Device* device) override;

    /**
     * @brief Shuts down the pass and releases resources.
     */
    void Shutdown() override;

    /**
     * @brief Executes the shadow map rendering.
     * 
     * Renders the specified mesh into the shadow map depth buffer.
     * 
     * @param context Pointer to the ID3D11DeviceContext.
     * @param spotData Parameters of the spotlight used for light matrix calculation.
     * @param mesh Pointer to the mesh to render (usually the stage).
     * @param stageOffset Vertical offset for the mesh.
     */
    void Execute(ID3D11DeviceContext* context,
                 const SpotlightData& spotData,
                 Mesh* mesh,
                 float stageOffset);

    /**
     * @brief Gets the shader resource view of the shadow map.
     * @return Pointer to the shadow map SRV.
     */
    ID3D11ShaderResourceView* GetShadowSRV() const { return m_shadowSRV.Get(); }

    /**
     * @brief Gets the sampler state used for shadow comparison.
     * @return Pointer to the shadow sampler state.
     */
    ID3D11SamplerState* GetShadowSampler() const { return m_shadowSampler.Get(); }

    /**
     * @brief Gets the light's view-projection matrix used during the last Execute call.
     * @return The 4x4 light view-projection matrix.
     */
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
