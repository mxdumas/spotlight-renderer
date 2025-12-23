#pragma once

#include <d3d11.h>

class GraphicsDevice;

// Forward declarations for scene data that passes may need
struct MatrixBuffer;
struct SpotlightData;
struct VolumetricBuffer;
struct MaterialBuffer;
struct CeilingLightsData;

// Base interface for all render passes
// Each pass encapsulates a specific stage of the rendering pipeline
class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    // Initialize pass-specific resources (shaders, buffers, states)
    virtual bool Initialize(ID3D11Device* device) = 0;

    // Clean up resources
    virtual void Shutdown() = 0;
};
