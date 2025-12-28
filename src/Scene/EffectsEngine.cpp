#include "EffectsEngine.h"
#include <cmath>
#include "Spotlight.h"

void EffectsEngine::Update(std::vector<Spotlight> &spotlights, float time)
{
    if (!m_enabled)
        return;

    float t = time * m_speed;

    for (size_t i = 0; i < spotlights.size(); ++i)
    {
        auto &light = spotlights[i];
        float phase = static_cast<float>(i) * 0.5f;

        // Pan animation
        if (m_panEnabled)
        {
            float pan = std::sin(t * PAN_SPEED + phase) * PAN_AMPLITUDE;
            light.SetPan(pan);
        }

        // Tilt animation
        if (m_tiltEnabled)
        {
            float tilt = std::cos(t * TILT_SPEED + phase) * TILT_AMPLITUDE + TILT_OFFSET;
            light.SetTilt(tilt);
        }

        // Rainbow color chase
        if (m_rainbowEnabled)
        {
            float hue = std::fmod(t * RAINBOW_SPEED + static_cast<float>(i) * 0.25f, 1.0f);

            // HSV to RGB (simplified)
            float r = 0, g = 0, b = 0;
            float h = hue * 6.0f;
            float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);
            if (h < 1.0f)
            {
                r = 1;
                g = x;
            }
            else if (h < 2.0f)
            {
                r = x;
                g = 1;
            }
            else if (h < 3.0f)
            {
                g = 1;
                b = x;
            }
            else if (h < 4.0f)
            {
                g = x;
                b = 1;
            }
            else if (h < 5.0f)
            {
                r = x;
                b = 1;
            }
            else
            {
                r = 1;
                b = x;
            }

            light.SetColor(r, g, b);
        }

        // Gobo rotation
        if (m_goboRotationEnabled)
        {
            light.SetGoboRotation(t * GOBO_SPEED + phase);
        }
    }
}
