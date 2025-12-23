#pragma once

#include <DirectXMath.h>
#include "../Core/Config.h"

/**
 * @struct PointLight
 * @brief Represents a single point light source for GPU processing.
 */
struct PointLight {
    DirectX::XMFLOAT4 pos;   ///< xyz: world position, w: range (attenuation).
    DirectX::XMFLOAT4 color; ///< xyz: RGB color, w: intensity.
};

/**
 * @struct CeilingLightsData
 * @brief GPU-aligned structure for the ceiling lights constant buffer.
 */
__declspec(align(16)) struct CeilingLightsData {
    PointLight lights[Config::CeilingLights::TOTAL_LIGHTS]; ///< Array of point lights.
    DirectX::XMFLOAT4 ambient;                             ///< Global ambient light (fill).
};

/**
 * @class CeilingLights
 * @brief Manages a collection of ceiling-mounted point lights.
 * 
 * This class handles high-level configuration of intensity, color, and ambient fill,
 * and prepares the data for use in shaders via the CeilingLightsData structure.
 */
class CeilingLights {
public:
    /**
     * @brief Constructor for the CeilingLights class.
     * Initializes default light positions and colors based on configuration.
     */
    CeilingLights();

    /**
     * @brief Sets the intensity of all ceiling lights.
     * @param intensity The brightness multiplier.
     */
    void SetIntensity(float intensity);

    /**
     * @brief Sets the global ambient fill level.
     * @param fill The ambient light intensity.
     */
    void SetAmbient(float fill);

    /**
     * @brief Sets the base RGB color for all ceiling lights.
     * 
     * @param r Red component (0-1).
     * @param g Green component (0-1).
     * @param b Blue component (0-1).
     */
    void SetColor(float r, float g, float b);

    /**
     * @brief Gets the current intensity multiplier.
     * @return Intensity value.
     */
    float GetIntensity() const { return m_intensity; }

    /**
     * @brief Gets the current ambient fill level.
     * @return Ambient fill value.
     */
    float GetAmbient() const { return m_ambientFill; }

    /**
     * @brief Updates the internal GPU data structure with current parameters.
     * Should be called before uploading to the GPU constant buffer.
     */
    void Update();

    /**
     * @brief Gets the GPU-ready data structure for constant buffer updates.
     * @return Const reference to CeilingLightsData.
     */
    const CeilingLightsData& GetGPUData() const { return m_data; }

private:
    CeilingLightsData m_data;
    float m_intensity;
    float m_ambientFill;
    DirectX::XMFLOAT3 m_color;
};
