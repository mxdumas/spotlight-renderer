#pragma once

#include <d3d11.h>

class GraphicsDevice;

// Forward declarations for scene data that passes may need
struct MatrixBuffer;
struct SpotlightData;
struct VolumetricBuffer;
struct MaterialBuffer;
struct CeilingLightsData;

/**
 * @class IRenderPass
 * @brief Base interface for all render passes in the pipeline.
 * 
 * Each render pass encapsulates a specific stage of the rendering process,
 * such as shadow mapping, scene rendering, or post-processing.
 */
class IRenderPass {
public:
    /**
     * @brief Virtual destructor for the IRenderPass interface.
     */
    virtual ~IRenderPass() = default;

    /**
     * @brief Initializes pass-specific resources like shaders, buffers, and state objects.
     * 
     * @param device Pointer to the ID3D11Device used for resource creation.
     * @return true if initialization was successful, false otherwise.
     */
    virtual bool Initialize(ID3D11Device* device) = 0;

    /**
     * @brief Shuts down the render pass and releases all allocated resources.
     */
    virtual void Shutdown() = 0;
};
