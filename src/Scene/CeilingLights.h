#pragma once

#include <DirectXMath.h>
#include "../Core/Config.h"

// Single point light for GPU
struct PointLight {
    DirectX::XMFLOAT4 pos;   // xyz: position, w: range
    DirectX::XMFLOAT4 color; // xyz: color, w: intensity
};

// GPU-aligned structure for shader constant buffer
__declspec(align(16)) struct CeilingLightsData {
    PointLight lights[Config::CeilingLights::TOTAL_LIGHTS];
    DirectX::XMFLOAT4 ambient;
};

// High-level ceiling lights management
class CeilingLights {
public:
    CeilingLights();

    // Configuration
    void SetIntensity(float intensity);
    void SetAmbient(float fill);
    void SetColor(float r, float g, float b);

    // Accessors
    float GetIntensity() const { return m_intensity; }
    float GetAmbient() const { return m_ambientFill; }

    // Update GPU data (call before uploading to constant buffer)
    void Update();

    // Get GPU data for constant buffer
    const CeilingLightsData& GetGPUData() const { return m_data; }

private:
    CeilingLightsData m_data;
    float m_intensity;
    float m_ambientFill;
    DirectX::XMFLOAT3 m_color;
};
