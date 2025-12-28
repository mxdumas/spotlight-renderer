#pragma once

#include <DirectXMath.h>
#include <memory>
#include "../Core/Config.h"

namespace SceneGraph
{
class Node;
}

/**
 * @struct SpotlightData
 * @brief GPU-aligned structure for the spotlight constant buffer.
 */
__declspec(align(16)) struct SpotlightData
{
    DirectX::XMMATRIX lightViewProj; ///< Light's view-projection matrix for shadow mapping.
    DirectX::XMFLOAT4 posRange;      ///< xyz: position, w: range.
    DirectX::XMFLOAT4 dirAngle;      ///< xyz: direction, w: spot angle (not used in current shader).
    DirectX::XMFLOAT4 colorInt;      ///< xyz: RGB color, w: intensity.
    DirectX::XMFLOAT4 coneGobo;      ///< x: beam angle, y: field angle, z: rotation, w: unused.
    DirectX::XMFLOAT4 goboOff;       ///< xy: gobo texture offset (shake), zw: unused.
};

/**
 * @class Spotlight
 * @brief Represents a high-end stage lighting fixture (spotlight).
 *
 * Manages position, orientation, color (RGB or CMY), beam angles, and gobo properties
 * for a single spotlight source.
 */
class Spotlight
{
public:
    /**
     * @brief Constructor for the Spotlight class.
     * Initializes the spotlight with default professional lighting parameters.
     */
    Spotlight();

    /**
     * @brief Sets the world position of the spotlight.
     * @param pos The position as XMFLOAT3.
     */
    void SetPosition(const DirectX::XMFLOAT3 &pos);

    /**
     * @brief Sets the world position of the spotlight using individual coordinates.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     */
    void SetPosition(float x, float y, float z);

    /**
     * @brief Sets the direction vector of the spotlight.
     * @param dir The normalized direction vector.
     */
    void SetDirection(const DirectX::XMFLOAT3 &dir);

    /**
     * @brief Sets the RGB color of the light.
     *
     * @param r Red (0-1).
     * @param g Green (0-1).
     * @param b Blue (0-1).
     */
    void SetColor(float r, float g, float b);

    /**
     * @brief Sets the color using CMY (Cyan, Magenta, Yellow) subtractive mixing.
     *
     * @param c Cyan (0-1).
     * @param m Magenta (0-1).
     * @param y Yellow (0-1).
     */
    void SetColorFromCMY(float c, float m, float y);

    /**
     * @brief Sets the brightness intensity of the light.
     * @param intensity Brightness multiplier.
     */
    void SetIntensity(float intensity);

    /**
     * @brief Sets the maximum range (distance) of the light.
     * @param range Maximum reach in world units.
     */
    void SetRange(float range);

    /**
     * @brief Sets the beam angle (inner cone where light is at full intensity).
     * @param beam Angle in radians.
     */
    void SetBeamAngle(float beam);

    /**
     * @brief Sets the field angle (outer cone where light falls off to zero).
     * @param field Angle in radians.
     */
    void SetFieldAngle(float field);

    /**
     * @brief Sets the rotation angle for the gobo texture.
     * @param rotation Angle in radians.
     */
    void SetGoboRotation(float rotation);

    /**
     * @brief Sets the active gobo index (slot number).
     * @param index 0-based index of the gobo in the texture array.
     */
    void SetGoboIndex(int index);

    /**
     * @brief Sets the intensity of the gobo "shake" animation.
     * @param amount Maximum displacement for the shake effect.
     */
    void SetGoboShake(float amount);

    /**
     * @brief Sets the pan angle in degrees.
     * @param degrees Pan angle.
     */
    void SetPan(float degrees);

    /**
     * @brief Sets the tilt angle in degrees.
     * @param degrees Tilt angle.
     */
    void SetTilt(float degrees);

    /**
     * @brief Gets the current pan angle.
     * @return Pan in degrees.
     */
    [[nodiscard]] float GetPan() const
    {
        return m_pan;
    }

    /**
     * @brief Gets the current tilt angle.
     * @return Tilt in degrees.
     */
    [[nodiscard]] float GetTilt() const
    {
        return m_tilt;
    }

    /**
     * @brief Links scene graph nodes to this spotlight for hierarchical animation.
     * @param pan Node responsible for panning (usually Yoke).
     * @param tilt Node responsible for tilting (usually Head).
     * @param beam Node representing the light emission point.
     */
    void LinkNodes(std::shared_ptr<SceneGraph::Node> pan, std::shared_ptr<SceneGraph::Node> tilt,
                   std::shared_ptr<SceneGraph::Node> beam);

    /**
     * @brief Synchronizes the spotlight's position and direction with its linked scene graph nodes.
     */
    void UpdateFromNodes();

    /**
     * @brief Gets the current world position.
     * @return Position as XMFLOAT3.
     */
    [[nodiscard]] DirectX::XMFLOAT3 GetPosition() const;

    /**
     * @brief Gets the current direction vector.
     * @return Normalized direction as XMFLOAT3.
     */
    [[nodiscard]] DirectX::XMFLOAT3 GetDirection() const;

    /**
     * @brief Gets the maximum range.
     * @return Range value.
     */
    [[nodiscard]] float GetRange() const
    {
        return m_data.posRange.w;
    }

    /**
     * @brief Gets the current intensity.
     * @return Intensity value.
     */
    [[nodiscard]] float GetIntensity() const
    {
        return m_data.colorInt.w;
    }

    /**
     * @brief Gets the beam angle.
     * @return Beam angle in radians.
     */
    [[nodiscard]] float GetBeamAngle() const
    {
        return m_data.coneGobo.x;
    }

    /**
     * @brief Gets the field angle.
     * @return Field angle in radians.
     */
    [[nodiscard]] float GetFieldAngle() const
    {
        return m_data.coneGobo.y;
    }

    /**
     * @brief Gets the gobo rotation.
     * @return Rotation in radians.
     */
    [[nodiscard]] float GetGoboRotation() const
    {
        return m_data.coneGobo.z;
    }

    /**
     * @brief Gets the gobo index.
     * @return Index value.
     */
    [[nodiscard]] int GetGoboIndex() const
    {
        return static_cast<int>(m_data.coneGobo.w);
    }

    /**
     * @brief Gets the current gobo shake intensity.
     * @return Shake amount.
     */
    [[nodiscard]] float GetGoboShake() const
    {
        return m_goboShakeAmount;
    }

    /**
     * @brief Updates the gobo shake animation based on elapsed time.
     * @param time Total elapsed time in seconds.
     */
    void UpdateGoboShake(float time);

    /**
     * @brief Recomputes the light's view-projection matrix for shadow mapping.
     */
    void UpdateLightMatrix();

    /**
     * @brief Gets the GPU-ready data structure for constant buffer updates.
     * @return Const reference to SpotlightData.
     */
    [[nodiscard]] const SpotlightData &GetGPUData() const
    {
        return m_data;
    }

    /**
     * @brief Gets a mutable reference to the GPU data structure.
     * @return Reference to SpotlightData.
     */
    SpotlightData &GetGPUDataMutable()
    {
        return m_data;
    }

private:
    SpotlightData m_data;
    float m_goboShakeAmount{0.0f};

    // GDTF Animation
    float m_pan{0.0f};
    float m_tilt{0.0f};
    std::shared_ptr<SceneGraph::Node> m_panNode;
    std::shared_ptr<SceneGraph::Node> m_tiltNode;
    std::shared_ptr<SceneGraph::Node> m_beamNode;
};
