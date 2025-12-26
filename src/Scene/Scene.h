#pragma once

#include <memory>
#include <vector>
#include "../Core/Config.h"
#include "../GDTF/GDTFLoader.h"
#include "../GDTF/GDTFParser.h"
#include "../Resources/Mesh.h"
#include "../Resources/Texture.h"
#include "Camera.h"
#include "CeilingLights.h"
#include "Spotlight.h"

// Scene container class
// Holds all scene state: camera, lights, meshes, materials
// Supports multiple spotlights for future extensibility
/**
 * @class Scene
 * @brief Container class that holds all scene state, including camera, lights, meshes, and materials.
 *
 * The Scene class manages the lifecycle and updates of all scene components and supports
 * multiple spotlights for future extensibility.
 */
class Scene
{
public:
    /**
     * @brief Constructor for the Scene class.
     * Initializes default values for camera, lights, and materials.
     */
    Scene();

    /**
     * @brief Default destructor for the Scene class.
     */
    ~Scene() = default;

    /**
     * @brief Initializes scene resources like meshes and textures.
     *
     * @param device Pointer to the ID3D11Device used for resource creation.
     * @return true if initialization was successful, false otherwise.
     */
    bool Initialize(ID3D11Device *device);

    /**
     * @brief Updates the scene state for a single frame.
     *
     * @param deltaTime The time elapsed since the last frame in seconds.
     */
    void Update(float deltaTime);

    /**
     * @brief Gets a reference to the active camera.
     * @return Reference to the Camera object.
     */
    Camera &GetCamera()
    {
        return m_camera;
    }

    /**
     * @brief Gets a const reference to the active camera.
     * @return Const reference to the Camera object.
     */
    const Camera &GetCamera() const
    {
        return m_camera;
    }

    /**
     * @brief Gets a reference to the camera's orbit distance.
     * @return Reference to the distance value.
     */
    float &CamDistance()
    {
        return m_camDistance;
    }

    /**
     * @brief Gets a reference to the camera's pitch angle.
     * @return Reference to the pitch value in radians.
     */
    float &CamPitch()
    {
        return m_camPitch;
    }

    /**
     * @brief Gets a reference to the camera's yaw angle.
     * @return Reference to the yaw value in radians.
     */
    float &CamYaw()
    {
        return m_camYaw;
    }

    /**
     * @brief Gets a reference to the camera's target position.
     * @return Reference to the target position XMFLOAT3.
     */
    DirectX::XMFLOAT3 &CamTarget()
    {
        return m_camTarget;
    }

    /**
     * @brief Computes the camera's world position based on orbit parameters.
     * @return The computed camera position as XMFLOAT3.
     */
    DirectX::XMFLOAT3 GetCameraPosition() const;

    /**
     * @brief Updates the camera's view matrix based on orbit parameters.
     */
    void UpdateCamera();

    /**
     * @brief Gets a reference to the primary spotlight.
     * @return Reference to the first Spotlight in the collection.
     */
    Spotlight &GetSpotlight()
    {
        return m_spotlights[0];
    }

    /**
     * @brief Gets a const reference to the primary spotlight.
     * @return Const reference to the first Spotlight in the collection.
     */
    const Spotlight &GetSpotlight() const
    {
        return m_spotlights[0];
    }

    /**
     * @brief Gets a reference to the list of all spotlights in the scene.
     * @return Reference to the vector of Spotlight objects.
     */
    std::vector<Spotlight> &GetSpotlights()
    {
        return m_spotlights;
    }

    /**
     * @brief Gets a const reference to the list of all spotlights in the scene.
     * @return Const reference to the vector of Spotlight objects.
     */
    const std::vector<Spotlight> &GetSpotlights() const
    {
        return m_spotlights;
    }

    /**
     * @brief Adds a new spotlight to the scene.
     * @param light The Spotlight object to add.
     */
    void AddSpotlight(const Spotlight &light);

    /**
     * @brief Removes a spotlight from the scene by its index.
     * @param index The index of the spotlight to remove.
     */
    void RemoveSpotlight(size_t index);

    /**
     * @brief Gets a reference to the ceiling lights.
     * @return Reference to the CeilingLights object.
     */
    CeilingLights &GetCeilingLights()
    {
        return m_ceilingLights;
    }

    /**
     * @brief Gets a const reference to the ceiling lights.
     * @return Const reference to the CeilingLights object.
     */
    const CeilingLights &GetCeilingLights() const
    {
        return m_ceilingLights;
    }

    /**
     * @brief Gets the stage mesh pointer.
     * @return Pointer to the stage Mesh.
     */
    Mesh *GetStageMesh()
    {
        return m_stageMesh.get();
    }

    /**
     * @brief Gets the stage mesh pointer (const).
     * @return Const pointer to the stage Mesh.
     */
    const Mesh *GetStageMesh() const
    {
        return m_stageMesh.get();
    }

    /**
     * @brief Gets the gobo texture pointer.
     * @return Pointer to the gobo Texture.
     */
    Texture *GetGoboTexture()
    {
        return m_goboTexture.get();
    }

    /**
     * @brief Gets the gobo texture pointer (const).
     * @return Const pointer to the gobo Texture.
     */
    const Texture *GetGoboTexture() const
    {
        return m_goboTexture.get();
    }

    /**
     * @brief Gets the list of anchor positions.
     * @return Const reference to the vector of anchor positions.
     */
    const std::vector<DirectX::XMFLOAT3> &GetAnchorPositions() const
    {
        return m_anchorPositions;
    }

    /**
     * @brief Gets the position where the spotlight fixture is mounted.
     * @return Reference to the fixture position XMFLOAT3.
     */
    const DirectX::XMFLOAT3 &GetFixturePosition() const
    {
        return m_fixturePos;
    }

    /**
     * @brief Gets the collection of GDTF fixture placement nodes.
     * @return Const reference to the vector of shared pointers to SceneGraph::Nodes.
     */
    const std::vector<std::shared_ptr<SceneGraph::Node>> &GetFixtureNodes() const
    {
        return m_fixtureNodes;
    }

    /**
     * @brief Gets the list of gobo slot names for the current wheel.
     * @return Const reference to the vector of strings.
     */
    const std::vector<std::string> &GetGoboSlotNames() const
    {
        return m_goboSlotNames;
    }

    /**
     * @brief Gets the vertical offset of the stage.
     * @return The stage offset value.
     */
    float GetStageOffset() const
    {
        return m_stageOffset;
    }

    /**
     * @brief Gets a reference to the room's specular intensity.
     * @return Reference to the specular value.
     */
    float &RoomSpecular()
    {
        return m_roomSpecular;
    }

    /**
     * @brief Gets a reference to the room's shininess (roughness).
     * @return Reference to the shininess value.
     */
    float &RoomShininess()
    {
        return m_roomShininess;
    }

    /**
     * @brief Gets the room's specular intensity.
     * @return The specular value.
     */
    float GetRoomSpecular() const
    {
        return m_roomSpecular;
    }

    /**
     * @brief Gets the room's shininess (roughness).
     * @return The shininess value.
     */
    float GetRoomShininess() const
    {
        return m_roomShininess;
    }

    /**
     * @brief Gets a reference to the CMY color mixing toggle.
     * @return Reference to the boolean flag.
     */
    bool &UseCMY()
    {
        return m_useCMY;
    }

    /**
     * @brief Gets a reference to the CMY color values.
     * @return Reference to the XMFLOAT3 representing Cyan, Magenta, Yellow.
     */
    DirectX::XMFLOAT3 &CMY()
    {
        return m_cmy;
    }

    /**
     * @brief Checks if CMY color mixing is enabled.
     * @return true if CMY is used, false if RGB is used.
     */
    bool GetUseCMY() const
    {
        return m_useCMY;
    }

    /**
     * @brief Gets the CMY color values.
     * @return Const reference to the XMFLOAT3 representing CMY.
     */
    const DirectX::XMFLOAT3 &GetCMY() const
    {
        return m_cmy;
    }

    /**
     * @brief Gets the total elapsed time in the scene.
     * @return Total time in seconds.
     */
    float GetTime() const
    {
        return m_time;
    }

    /**
     * @brief Checks if Demo Mode is active.
     * @return true if demo mode is enabled.
     */
    bool &DemoMode()
    {
        return m_demoMode;
    }

private:
    // Camera
    Camera m_camera;
    float m_camDistance;
    float m_camPitch;
    float m_camYaw;
    DirectX::XMFLOAT3 m_camTarget;

    // Lights
    std::vector<Spotlight> m_spotlights;
    CeilingLights m_ceilingLights;

    // Meshes and textures
    std::unique_ptr<Mesh> m_stageMesh;
    std::unique_ptr<Texture> m_goboTexture;

    // Derived from mesh
    std::vector<DirectX::XMFLOAT3> m_anchorPositions;
    DirectX::XMFLOAT3 m_fixturePos;
    float m_stageOffset{0.0f};

    // GDTF Fixtures
    std::vector<std::shared_ptr<SceneGraph::Node>> m_fixtureNodes;
    std::vector<std::string> m_goboSlotNames;

    // Room materials
    float m_roomSpecular;
    float m_roomShininess;

    // CMY color state
    bool m_useCMY{false};
    DirectX::XMFLOAT3 m_cmy;

    // Demo Mode
    bool m_demoMode{true};

    // Time
    float m_time{0.0f};
};
