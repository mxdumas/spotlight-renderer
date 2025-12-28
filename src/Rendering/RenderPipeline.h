#pragma once

#include <d3d11.h>
#include <memory>
#include <vector>
#include <wrl/client.h>
#include "../Core/ConstantBuffer.h"
#include "../Scene/Camera.h"
#include "../Scene/CeilingLights.h"
#include "../Scene/Node.h"
#include "../Scene/Spotlight.h"
#include "Passes/BlurPass.h"
#include "Passes/CompositePass.h"
#include "Passes/FXAAPass.h"
#include "Passes/ScenePass.h"
#include "Passes/ShadowPass.h"
#include "Passes/VolumetricPass.h"
#include "RenderTarget.h"

using Microsoft::WRL::ComPtr;

class Mesh;
class Texture;

// Matrix buffer for main pass
__declspec(align(16)) struct PipelineMatrixBuffer
{
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX invViewProj;
    DirectX::XMFLOAT4 cameraPos;
};

/**
 * @struct RenderContext
 * @brief Holds all necessary context data for rendering a single frame.
 *
 * This structure aggregates camera, scene, and resource pointers needed by various render passes.
 */
struct RenderContext
{
    // Camera
    Camera *camera;              ///< Pointer to the active camera.
    DirectX::XMFLOAT3 cameraPos; ///< Current camera position in world space.

    // Scene data
    std::vector<DirectX::XMFLOAT3> anchorPositions;              ///< Positions of fixture anchors.
    std::vector<std::shared_ptr<SceneGraph::Node>> fixtureNodes; ///< Fixture hierarchies.
    Spotlight *spotlight;                                        ///< Pointer to the main spotlight.
    std::vector<Spotlight> *spotlights;                          ///< Pointer to the list of all spotlights.
    CeilingLights *ceilingLights;                                ///< Pointer to the ceiling lights collection.
    Mesh *stageMesh;                                             ///< Pointer to the stage geometry mesh.
    Texture *goboTexture;                                        ///< Pointer to the gobo texture for the spotlight.
    float stageOffset;                                           ///< Vertical offset for the stage.
    float time;                                                  ///< Total elapsed time for animations.

    // Room geometry (owned by caller)
    ID3D11Buffer *roomVB; ///< Pointer to the room's vertex buffer.
    ID3D11Buffer *roomIB; ///< Pointer to the room's index buffer.

    // Room material
    float roomSpecular;  ///< Specular intensity of the room material.
    float roomShininess; ///< Shininess (roughness) of the room material.

    // Depth buffer for scene pass
    ID3D11DepthStencilView *depthStencilView; ///< View for the depth-stencil buffer.
    ID3D11ShaderResourceView *depthSRV;       ///< Shader resource view for the depth buffer.

    // Final output
    ID3D11RenderTargetView *backBufferRTV; ///< The main back buffer render target view.
};

/**
 * @class RenderPipeline
 * @brief Orchestrates all render passes in the correct order to produce a final frame.
 *
 * The RenderPipeline manages the lifecycle and execution of various render passes,
 * including shadow mapping, scene rendering, volumetric lighting, and post-processing.
 */
class RenderPipeline
{
public:
    /**
     * @brief Default constructor for the RenderPipeline class.
     */
    RenderPipeline() = default;

    /**
     * @brief Destructor for the RenderPipeline class.
     */
    ~RenderPipeline();

    /**
     * @brief Initializes all render passes, shared resources, and constant buffers.
     *
     * @param device Pointer to the ID3D11Device used for resource creation.
     * @return true if initialization was successful, false otherwise.
     */
    bool Initialize(ID3D11Device *device);

    /**
     * @brief Shuts down the pipeline and releases all internal resources.
     */
    void Shutdown();

    /**
     * @brief Executes the full rendering pipeline.
     *
     * This method runs all enabled render passes in the appropriate sequence.
     *
     * @param context Pointer to the ID3D11DeviceContext used for rendering commands.
     * @param ctx The RenderContext containing scene and camera data for the current frame.
     */
    void Render(ID3D11DeviceContext *context, const RenderContext &ctx);

    /**
     * @brief Enables or disables FXAA post-processing.
     * @param enabled Set to true to enable FXAA, false to disable.
     */
    void SetFXAAEnabled(bool enabled)
    {
        m_enableFXAA = enabled;
    }

    /**
     * @brief Checks if FXAA post-processing is enabled.
     * @return true if FXAA is enabled, false otherwise.
     */
    [[nodiscard]] bool IsFXAAEnabled() const
    {
        return m_enableFXAA;
    }

    /**
     * @brief Enables or disables blurring of the volumetric lighting buffer.
     * @param enabled Set to true to enable volumetric blur, false to disable.
     */
    void SetVolumetricBlurEnabled(bool enabled)
    {
        m_enableVolBlur = enabled;
    }

    /**
     * @brief Checks if volumetric blur is enabled.
     * @return true if volumetric blur is enabled, false otherwise.
     */
    [[nodiscard]] bool IsVolumetricBlurEnabled() const
    {
        return m_enableVolBlur;
    }

    /**
     * @brief Sets the number of blur passes to perform on the volumetric buffer.
     * @param passes The number of blur iterations.
     */
    void SetBlurPasses(int passes)
    {
        m_blurPasses = passes;
    }

    /**
     * @brief Gets the current number of blur passes.
     * @return The number of blur iterations.
     */
    [[nodiscard]] int GetBlurPasses() const
    {
        return m_blurPasses;
    }

    /**
     * @brief Provides access to the volumetric pass parameters for modification (e.g., via UI).
     * @return A reference to the VolumetricBuffer struct containing parameters.
     */
    VolumetricBuffer &GetVolumetricParams()
    {
        return m_volumetricPass->GetParams();
    }

    /**
     * @brief Provides read-only access to the volumetric pass parameters.
     * @return A const reference to the VolumetricBuffer struct.
     */
    [[nodiscard]] const VolumetricBuffer &GetVolumetricParams() const
    {
        return m_volumetricPass->GetParams();
    }

private:
    /**
     * @brief Executes the shadow mapping pass.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param ctx The RenderContext for the current frame.
     */
    void RenderShadowPass(ID3D11DeviceContext *context, const RenderContext &ctx);

    /**
     * @brief Executes the main scene rendering pass.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param ctx The RenderContext for the current frame.
     */
    void RenderScenePass(ID3D11DeviceContext *context, const RenderContext &ctx);

    /**
     * @brief Helper to recursively render scene graph nodes.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param node The node to render.
     * @param mb Matrix buffer to update for each node.
     */
    void RenderNodeRecursive(ID3D11DeviceContext *context, const std::shared_ptr<SceneGraph::Node> &node,
                             PipelineMatrixBuffer &mb);

    /**
     * @brief Executes the volumetric lighting pass.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param ctx The RenderContext for the current frame.
     */
    void RenderVolumetricPass(ID3D11DeviceContext *context, const RenderContext &ctx);

    /**
     * @brief Executes the blur post-processing pass on the volumetric buffer.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     */
    void RenderBlurPass(ID3D11DeviceContext *context);

    /**
     * @brief Executes the composite pass, combining scene and volumetric lighting.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     */
    void RenderCompositePass(ID3D11DeviceContext *context);

    /**
     * @brief Executes the final pass, including FXAA if enabled, and outputs to the back buffer.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param ctx The RenderContext for the current frame.
     */
    void RenderFinalPass(ID3D11DeviceContext *context, const RenderContext &ctx);

    /**
     * @brief Helper method to set the viewport for a specific pass.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param width The width of the viewport.
     * @param height The height of the viewport.
     */
    void SetupViewport(ID3D11DeviceContext *context, int width, int height);

    /**
     * @brief Unbinds all shader resource views from the pixel shader to avoid binding conflicts.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     */
    void ClearShaderResources(ID3D11DeviceContext *context);

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
    ComPtr<ID3D11Buffer> m_debugSphereVB;
    ComPtr<ID3D11Buffer> m_debugSphereIB;
    uint32_t m_debugSphereIndexCount = 0;

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
    ID3D11Device *m_device = nullptr;
};