#pragma once

#include "Spotlight.h"
#include "CeilingLights.h"
#include "../Camera.h"
#include "../Mesh.h"
#include "../Texture.h"
#include "../Core/Config.h"
#include <memory>
#include <vector>

// Scene container class
// Holds all scene state: camera, lights, meshes, materials
// Supports multiple spotlights for future extensibility
class Scene {
public:
    Scene();
    ~Scene() = default;

    // Initialize scene resources (mesh, textures)
    bool Initialize(ID3D11Device* device);

    // Update scene state (called each frame)
    void Update(float deltaTime);

    // Camera access
    Camera& GetCamera() { return m_camera; }
    const Camera& GetCamera() const { return m_camera; }

    // Camera orbit parameters
    float& CamDistance() { return m_camDistance; }
    float& CamPitch() { return m_camPitch; }
    float& CamYaw() { return m_camYaw; }
    DirectX::XMFLOAT3& CamTarget() { return m_camTarget; }

    // Get computed camera position from orbit parameters
    DirectX::XMFLOAT3 GetCameraPosition() const;

    // Update camera from orbit parameters
    void UpdateCamera();

    // Spotlight access (primary spotlight for now)
    Spotlight& GetSpotlight() { return m_spotlights[0]; }
    const Spotlight& GetSpotlight() const { return m_spotlights[0]; }

    // Multi-spotlight support
    std::vector<Spotlight>& GetSpotlights() { return m_spotlights; }
    const std::vector<Spotlight>& GetSpotlights() const { return m_spotlights; }
    void AddSpotlight(const Spotlight& light);
    void RemoveSpotlight(size_t index);

    // Ceiling lights
    CeilingLights& GetCeilingLights() { return m_ceilingLights; }
    const CeilingLights& GetCeilingLights() const { return m_ceilingLights; }

    // Stage mesh
    Mesh* GetStageMesh() { return m_stageMesh.get(); }
    const Mesh* GetStageMesh() const { return m_stageMesh.get(); }

    // Gobo texture
    Texture* GetGoboTexture() { return m_goboTexture.get(); }
    const Texture* GetGoboTexture() const { return m_goboTexture.get(); }

    // Fixture position (from mesh)
    const DirectX::XMFLOAT3& GetFixturePosition() const { return m_fixturePos; }

    // Stage offset
    float GetStageOffset() const { return m_stageOffset; }

    // Room materials
    float& RoomSpecular() { return m_roomSpecular; }
    float& RoomShininess() { return m_roomShininess; }
    float GetRoomSpecular() const { return m_roomSpecular; }
    float GetRoomShininess() const { return m_roomShininess; }

    // CMY color mixing state
    bool& UseCMY() { return m_useCMY; }
    DirectX::XMFLOAT3& CMY() { return m_cmy; }
    bool GetUseCMY() const { return m_useCMY; }
    const DirectX::XMFLOAT3& GetCMY() const { return m_cmy; }

    // Time
    float GetTime() const { return m_time; }

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
    DirectX::XMFLOAT3 m_fixturePos;
    float m_stageOffset;

    // Room materials
    float m_roomSpecular;
    float m_roomShininess;

    // CMY color state
    bool m_useCMY;
    DirectX::XMFLOAT3 m_cmy;

    // Time
    float m_time;
};
