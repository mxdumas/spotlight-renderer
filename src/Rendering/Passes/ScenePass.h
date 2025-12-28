#pragma once

#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include "../../Core/Config.h"
#include "../../Core/ConstantBuffer.h"
#include "../../Resources/Shader.h"
#include "../../Scene/Spotlight.h"
#include "IRenderPass.h"
#include "VolumetricPass.h" // For SpotlightArrayBuffer definition (or move definition to common header)

using Microsoft::WRL::ComPtr;

class Mesh;
class RenderTarget;

/**
 * @struct MaterialBuffer
 * @brief Material properties sent to the scene shader.
 */
__declspec(align(16)) struct MaterialBuffer
{
    DirectX::XMFLOAT4 color;      ///< Base diffuse color of the material.
    DirectX::XMFLOAT4 specParams; ///< Specular parameters: x: intensity, y: shininess, zw: unused.
};

/**
 * @class ScenePass
 * @brief Renders the static scene geometry, including the room and the stage.
 *
 * This pass handles the primary geometry rendering with standard lighting models,
 * preparing the scene for subsequent volumetric and post-processing effects.
 */
class ScenePass : public IRenderPass
{
public:
    /**
     * @brief Default constructor for the ScenePass class.
     */
    ScenePass() = default;

    /**
     * @brief Destructor for the ScenePass class.
     */
    ~ScenePass() override = default;

    /**
     * @brief Initializes the scene shaders, constant buffers, and rasterizer states.
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
     * @brief Sets the render target where the scene will be rendered.
     *
     * @param rt Pointer to the RenderTarget object.
     */
    void SetRenderTarget(RenderTarget *rt)
    {
        m_renderTarget = rt;
    }

    /**
     * @brief Executes the scene rendering.
     *
     * Renders both the room geometry and the stage mesh with the provided parameters.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param spotlights List of active spotlights.
     * @param dsv Depth-stencil view to use for depth testing.
     * @param room_vb Vertex buffer for the room geometry.
     * @param room_ib Index buffer for the room geometry.
     * @param stage_mesh Pointer to the stage mesh.
     * @param stage_offset Vertical offset for stage placement.
     * @param room_specular Specular intensity for the room material.
     * @param room_shininess Shininess exponent for the room material.
     */
    void Execute(ID3D11DeviceContext *context, const std::vector<Spotlight> &spotlights, ID3D11DepthStencilView *dsv,
                 ID3D11Buffer *room_vb, ID3D11Buffer *room_ib, Mesh *stage_mesh, float stage_offset,
                 float room_specular, float room_shininess);

    /**
     * @brief Gets the internal shader used by this pass.
     * @return Reference to the Shader object.
     */
    Shader &GetShader()
    {
        return m_basicShader;
    }

    /**
     * @brief Gets the material constant buffer for updating parameters.
     * @return Reference to the ConstantBuffer of MaterialBuffer.
     */
    ConstantBuffer<MaterialBuffer> &GetMaterialBuffer()
    {
        return m_materialBuffer;
    }

private:
    Shader m_basicShader;
    ConstantBuffer<MaterialBuffer> m_materialBuffer;
    ConstantBuffer<SpotlightArrayBuffer> m_spotlightArrayBuffer;
    RenderTarget *m_renderTarget = nullptr;

    // Rasterizer state for room (no culling)
    ComPtr<ID3D11RasterizerState> m_noCullState;
};
