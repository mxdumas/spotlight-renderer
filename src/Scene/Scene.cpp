#include "Scene.h"
#include <cmath>
#include <fstream>
#include <iostream>

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
    m_fixtureNodes.clear();

    // Load GDTF fixture model once
    GDTF::GDTFParser parser;
    std::shared_ptr<SceneGraph::Node> gdtfRoot;
    if (parser.load("data/fixtures/Martin_Professional@MAC_Viper_Performance@20230516NoMeas.gdtf"))
    {
        GDTF::GDTFLoader loader;
        gdtfRoot = loader.buildSceneGraph(device, parser);
    }

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

        // Add GDTF fixture node at this anchor
        if (gdtfRoot)
        {
            GDTF::GDTFLoader loader;
            auto instanceRoot = loader.buildSceneGraph(device, parser);
            if (instanceRoot)
            {
                // Placement node handles the world position (Anchor point)
                auto placementNode = std::make_shared<SceneGraph::Node>("Placement");
                placementNode->setTranslation(pos.x, pos.y, pos.z);
                
                // Orientation node handles the flip (so the fixture hangs correctly)
                auto orientationNode = std::make_shared<SceneGraph::Node>("Orientation");
                // Rotate 90 degrees around X (Pitch) to make GDTF "Forward" point down (-Y)
                orientationNode->setRotation(0.0f, DirectX::XM_PIDIV2, 0.0f);
                // Scale 2x
                orientationNode->setScale(2.0f, 2.0f, 2.0f);
                
                // Bring it up a little to touch the truss
                placementNode->setTranslation(pos.x, pos.y + 0.45f, pos.z);
                
                placementNode->addChild(orientationNode);
                orientationNode->addChild(instanceRoot);
                
                m_fixtureNodes.push_back(placementNode);
            }
        }
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

    // Update GDTF fixture hierarchies
    for (auto &node : m_fixtureNodes)
    {
        node->updateWorldMatrix();
    }
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
