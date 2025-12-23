#pragma once

#include <DirectXMath.h>
#include "../Core/Config.h"

// GPU-aligned structure for shader constant buffer
__declspec(align(16)) struct SpotlightData {
    DirectX::XMMATRIX lightViewProj; // c0-c3
    DirectX::XMFLOAT4 posRange;      // c4 (xyz: pos, w: range)
    DirectX::XMFLOAT4 dirAngle;      // c5 (xyz: dir, w: spotAngle - unused)
    DirectX::XMFLOAT4 colorInt;      // c6 (xyz: color, w: intensity)
    DirectX::XMFLOAT4 coneGobo;      // c7 (x: beam, y: field, z: rotation, w: unused)
    DirectX::XMFLOAT4 goboOff;       // c8 (xy: offset, zw: unused)
};

// High-level spotlight representation
class Spotlight {
public:
    Spotlight();

    // Configuration
    void SetPosition(const DirectX::XMFLOAT3& pos);
    void SetPosition(float x, float y, float z);
    void SetDirection(const DirectX::XMFLOAT3& dir);
    void SetColor(float r, float g, float b);
    void SetColorFromCMY(float c, float m, float y);
    void SetIntensity(float intensity);
    void SetRange(float range);
    void SetBeamAngle(float beam);
    void SetFieldAngle(float field);
    void SetGoboRotation(float rotation);
    void SetGoboShake(float amount);

    // Accessors
    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetDirection() const;
    float GetRange() const { return m_data.posRange.w; }
    float GetIntensity() const { return m_data.colorInt.w; }
    float GetBeamAngle() const { return m_data.coneGobo.x; }
    float GetFieldAngle() const { return m_data.coneGobo.y; }
    float GetGoboRotation() const { return m_data.coneGobo.z; }
    float GetGoboShake() const { return m_goboShakeAmount; }

    // Update gobo shake effect based on time
    void UpdateGoboShake(float time);

    // Compute light view-projection matrix
    void UpdateLightMatrix();

    // Get GPU data for constant buffer
    const SpotlightData& GetGPUData() const { return m_data; }
    SpotlightData& GetGPUDataMutable() { return m_data; }

private:
    SpotlightData m_data;
    float m_goboShakeAmount;
};
