#pragma once

#include <vector>

class Spotlight;

/**
 * @class EffectsEngine
 * @brief Manages demo effects for spotlights (pan/tilt animation, rainbow colors, gobo rotation).
 */
class EffectsEngine
{
public:
    EffectsEngine() = default;

    /**
     * @brief Updates all enabled effects on the given spotlights.
     * @param spotlights Vector of spotlights to apply effects to.
     * @param time Current time in seconds.
     */
    void Update(std::vector<Spotlight> &spotlights, float time);

    // Enable/disable
    bool &Enabled() { return m_enabled; }
    bool IsEnabled() const { return m_enabled; }

    // Effect toggles
    bool &PanEnabled() { return m_panEnabled; }
    bool &TiltEnabled() { return m_tiltEnabled; }
    bool &RainbowEnabled() { return m_rainbowEnabled; }
    bool &GoboRotationEnabled() { return m_goboRotationEnabled; }

    // Speed control
    float &Speed() { return m_speed; }
    float GetSpeed() const { return m_speed; }

private:
    bool m_enabled{true};
    float m_speed{1.0f};

    // Individual effect toggles
    bool m_panEnabled{true};
    bool m_tiltEnabled{true};
    bool m_rainbowEnabled{true};
    bool m_goboRotationEnabled{true};

    // Base speeds (will be multiplied by m_speed)
    static constexpr float PAN_SPEED = 0.6f;      // 50% faster than original 0.4f
    static constexpr float TILT_SPEED = 0.9f;     // 50% faster than original 0.6f
    static constexpr float RAINBOW_SPEED = 0.15f; // 50% faster than original 0.1f
    static constexpr float GOBO_SPEED = 0.375f;   // 50% faster than original 0.25f

    // Amplitude
    static constexpr float PAN_AMPLITUDE = 45.0f;
    static constexpr float TILT_AMPLITUDE = 30.0f;
    static constexpr float TILT_OFFSET = -20.0f;
};
