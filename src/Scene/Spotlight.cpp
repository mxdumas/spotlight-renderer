#include "Spotlight.h"
#include <cmath>
#include <cstring>

Spotlight::Spotlight()
{
    std::memset(&m_data, 0, sizeof(m_data));

    // Default values
    m_data.posRange = {0.0f, Config::Spotlight::DEFAULT_HEIGHT, 0.0f, Config::Spotlight::DEFAULT_RANGE};
    m_data.dirAngle = {0.0f, -1.0f, 0.0f, 0.0f}; // Pointing down
    m_data.colorInt = {1.0f, 1.0f, 1.0f, Config::Spotlight::DEFAULT_INTENSITY};
    m_data.coneGobo = {Config::Spotlight::DEFAULT_BEAM_ANGLE, Config::Spotlight::DEFAULT_FIELD_ANGLE, 0.0f, 0.0f};
    m_data.goboOff = {0.0f, 0.0f, 0.0f, 0.0f};
}

void Spotlight::SetPosition(const DirectX::XMFLOAT3 &pos)
{
    m_data.posRange.x = pos.x;
    m_data.posRange.y = pos.y;
    m_data.posRange.z = pos.z;
}

void Spotlight::SetPosition(float x, float y, float z)
{
    m_data.posRange.x = x;
    m_data.posRange.y = y;
    m_data.posRange.z = z;
}

void Spotlight::SetDirection(const DirectX::XMFLOAT3 &dir)
{
    // Normalize direction
    DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&dir);
    v = DirectX::XMVector3Normalize(v);
    DirectX::XMFLOAT3 normalized;
    DirectX::XMStoreFloat3(&normalized, v);

    m_data.dirAngle.x = normalized.x;
    m_data.dirAngle.y = normalized.y;
    m_data.dirAngle.z = normalized.z;
}

void Spotlight::SetColor(float r, float g, float b)
{
    m_data.colorInt.x = r;
    m_data.colorInt.y = g;
    m_data.colorInt.z = b;
}

void Spotlight::SetColorFromCMY(float c, float m, float y)
{
    m_data.colorInt.x = 1.0f - c;
    m_data.colorInt.y = 1.0f - m;
    m_data.colorInt.z = 1.0f - y;
}

void Spotlight::SetIntensity(float intensity)
{
    m_data.colorInt.w = intensity;
}

void Spotlight::SetRange(float range)
{
    m_data.posRange.w = range;
}

void Spotlight::SetBeamAngle(float beam)
{
    m_data.coneGobo.x = beam;
}

void Spotlight::SetFieldAngle(float field)
{
    m_data.coneGobo.y = field;
}

void Spotlight::SetGoboRotation(float rotation)
{
    m_data.coneGobo.z = rotation;
}

void Spotlight::SetGoboShake(float amount)
{
    m_goboShakeAmount = amount;
}

DirectX::XMFLOAT3 Spotlight::GetPosition() const
{
    return {m_data.posRange.x, m_data.posRange.y, m_data.posRange.z};
}

DirectX::XMFLOAT3 Spotlight::GetDirection() const
{
    return {m_data.dirAngle.x, m_data.dirAngle.y, m_data.dirAngle.z};
}

void Spotlight::UpdateGoboShake(float time)
{
    m_data.goboOff.x =
        std::sin(time * Config::Spotlight::SHAKE_FREQ_X) * m_goboShakeAmount * Config::Spotlight::SHAKE_SCALE;
    m_data.goboOff.y =
        std::cos(time * Config::Spotlight::SHAKE_FREQ_Y) * m_goboShakeAmount * Config::Spotlight::SHAKE_SCALE;
}

void Spotlight::UpdateLightMatrix()
{
    DirectX::XMVECTOR lPos = DirectX::XMVectorSet(m_data.posRange.x, m_data.posRange.y, m_data.posRange.z, 1.0f);
    DirectX::XMVECTOR lDir = DirectX::XMVector3Normalize(
        DirectX::XMVectorSet(m_data.dirAngle.x, m_data.dirAngle.y, m_data.dirAngle.z, 0.0f));
    DirectX::XMVECTOR lUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // If direction is too close to up vector, use a different up
    if (std::abs(DirectX::XMVectorGetY(lDir)) > 0.99f)
    {
        lUp = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    DirectX::XMMATRIX lView = DirectX::XMMatrixLookToLH(lPos, lDir, lUp);
    DirectX::XMMATRIX lProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1.0f, 0.1f, m_data.posRange.w);
    m_data.lightViewProj = DirectX::XMMatrixTranspose(lView * lProj);
}
