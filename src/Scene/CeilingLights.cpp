#include "CeilingLights.h"
#include <cstring>

CeilingLights::CeilingLights()
    : m_intensity(Config::CeilingLights::DEFAULT_INTENSITY), m_ambientFill(Config::Ambient::DEFAULT_FILL),
      m_color{1.0f, 1.0f, 1.0f}
{
    std::memset(&m_data, 0, sizeof(m_data));
    Update();
}

void CeilingLights::SetIntensity(float intensity)
{
    m_intensity = intensity;
}

void CeilingLights::SetAmbient(float fill)
{
    m_ambientFill = fill;
}

void CeilingLights::SetColor(float r, float g, float b)
{
    m_color = {r, g, b};
}

void CeilingLights::Update()
{
    int idx = 0;
    for (int z = 0; z < Config::CeilingLights::GRID_Z; ++z)
    {
        for (int x = 0; x < Config::CeilingLights::GRID_X; ++x)
        {
            float pos_x = Config::CeilingLights::X_START + (static_cast<float>(x) * Config::CeilingLights::X_SPACING);
            float pos_z = Config::CeilingLights::Z_START + (static_cast<float>(z) * Config::CeilingLights::Z_SPACING);

            m_data.lights[idx].pos = {pos_x, Config::CeilingLights::HEIGHT, pos_z, Config::CeilingLights::RANGE};
            m_data.lights[idx].color = {m_color.x, m_color.y, m_color.z,
                                        m_intensity * Config::CeilingLights::INTENSITY_MULTIPLIER};
            ++idx;
        }
    }

    // Normalize ambient to 0-1 range based on MAX_FILL
    float amb_val = m_ambientFill / Config::Ambient::MAX_FILL;
    m_data.ambient = {amb_val, amb_val, amb_val, 1.0f};
}
