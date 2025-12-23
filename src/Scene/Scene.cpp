#include "Scene.h"
#include <cmath>

Scene::Scene()
    : m_camDistance(Config::CameraDefaults::DISTANCE), m_camPitch(Config::CameraDefaults::PITCH),
      m_camYaw(Config::CameraDefaults::YAW), m_camTarget(0.0f, 0.0f, 0.0f),
      m_fixturePos(0.0f, Config::Spotlight::DEFAULT_HEIGHT, 0.0f), m_roomSpecular(Config::Materials::ROOM_SPECULAR),
      m_roomShininess(Config::Materials::ROOM_SHININESS), m_cmy(0.0f, 0.0f, 0.0f)
{
    // Create default spotlight
    m_spotlights.emplace_back();
}

bool Scene::Initialize(ID3D11Device *device)
{
    // Load stage mesh
    m_stageMesh = std::make_unique<Mesh>();
    if (!m_stageMesh->LoadFromOBJ(device, "data/models/stage.obj"))
    {
        return false;
    }

    // Calculate stage offset
    float stageMinY = m_stageMesh->GetMinY();
    m_stageOffset = Config::Room::FLOOR_Y - stageMinY;

    // Find anchor points from mesh
    m_anchorPositions.clear();
    for (const auto &shape : m_stageMesh->GetShapes())
    {
        if (shape.name.find("Anchor.") == 0)
        {
            DirectX::XMFLOAT3 pos = shape.center;
            pos.y += m_stageOffset;
            m_anchorPositions.push_back(pos);
        }
    }

    // Fallback if no anchors found
    if (m_anchorPositions.empty())
    {
        m_fixturePos = {0.0f, Config::Spotlight::DEFAULT_HEIGHT, 0.0f};
        for (const auto &shape : m_stageMesh->GetShapes())
        {
            if (shape.name == "Cylinder.000")
            {
                m_fixturePos = shape.center;
                break;
            }
        }
        m_fixturePos.y += m_stageOffset;
        m_anchorPositions.push_back(m_fixturePos);
    }
    else
    {
        m_fixturePos = m_anchorPositions[0];
    }

    // Load gobo texture
    m_goboTexture = std::make_unique<Texture>();
    if (!m_goboTexture->LoadFromFile(device, "data/models/gobo.jpg"))
    {
        // Fallback to stage.png
        m_goboTexture->LoadFromFile(device, "data/models/stage.png");
    }

    // Initialize spotlights
    m_spotlights.clear();
    for (const auto &pos : m_anchorPositions)
    {
        Spotlight light;
        light.SetPosition(pos);

        // Point towards center of stage
        DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&pos);
        DirectX::XMVECTOR dirVec = DirectX::XMVector3Normalize(DirectX::XMVectorNegate(posVec));
        DirectX::XMFLOAT3 dir;
        DirectX::XMStoreFloat3(&dir, dirVec);
        light.SetDirection(dir);

        m_spotlights.push_back(light);
    }

    // Initialize camera
    m_camera.SetPerspective(Config::CameraDefaults::FOV, Config::Display::ASPECT_RATIO,
                            Config::CameraDefaults::CLIP_NEAR, Config::CameraDefaults::CLIP_FAR);
    UpdateCamera();

    return true;
}

void Scene::Update(float deltaTime)
{
    m_time += deltaTime;
}

DirectX::XMFLOAT3 Scene::GetCameraPosition() const
{
    float camX = m_camDistance * cosf(m_camPitch) * sinf(m_camYaw);
    float camY = m_camDistance * sinf(m_camPitch);
    float camZ = -m_camDistance * cosf(m_camPitch) * cosf(m_camYaw);
    return {camX, camY, camZ};
}

void Scene::UpdateCamera()
{
    DirectX::XMFLOAT3 camPos = GetCameraPosition();
    m_camera.SetLookAt(camPos, m_camTarget, {0.0f, 1.0f, 0.0f});
}

void Scene::AddSpotlight(const Spotlight &light)
{
    m_spotlights.push_back(light);
}

void Scene::RemoveSpotlight(size_t index)
{
    if (index < m_spotlights.size() && m_spotlights.size() > 1)
    {
        m_spotlights.erase(m_spotlights.begin() + index);
    }
}
