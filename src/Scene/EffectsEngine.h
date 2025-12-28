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

    /// @brief Returns a reference to the enabled state for modification.
    bool &Enabled() { return m_enabled; }
    /// @brief Returns whether effects are enabled.
    [[nodiscard]] bool IsEnabled() const { return m_enabled; }

    /// @brief Returns a reference to the pan effect toggle.
    bool &PanEnabled() { return m_panEnabled; }
    /// @brief Returns a reference to the tilt effect toggle.
    bool &TiltEnabled() { return m_tiltEnabled; }
    /// @brief Returns a reference to the rainbow color effect toggle.
    bool &RainbowEnabled() { return m_rainbowEnabled; }
    /// @brief Returns a reference to the gobo rotation effect toggle.
    bool &GoboRotationEnabled() { return m_goboRotationEnabled; }

    /// @brief Returns a reference to the speed multiplier for modification.
    float &Speed() { return m_speed; }
    /// @brief Returns the current speed multiplier.
    [[nodiscard]] float GetSpeed() const { return m_speed; }

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
