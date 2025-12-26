#include "Scene.h"
#include <cmath>
#include <fstream>
#include <iostream>

Scene::Scene()
    : m_camDistance(Config::CameraDefaults::DISTANCE), m_camPitch(Config::CameraDefaults::PITCH),
      m_camYaw(Config::CameraDefaults::YAW), m_camTarget(Config::CameraDefaults::TARGET_X, Config::CameraDefaults::TARGET_Y, Config::CameraDefaults::TARGET_Z),
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

    // Load gobo texture - prefer GDTF gobos if available
    m_goboTexture = std::make_unique<Texture>();
    m_goboSlotNames.clear();
    auto gobo_images = parser.extractGoboImages();
    if (!gobo_images.empty())
    {
        if (m_goboTexture->CreateTextureArray(device, gobo_images))
        {
            // Populate gobo slot names from GDTF
            m_goboSlotNames.push_back("Open");
            for (const auto &wheel : parser.getGoboWheels())
            {
                if (wheel.name.find("Gobo") == std::string::npos)
                    continue;
                for (const auto &slot : wheel.slots)
                {
                    if (!slot.media_file_name.empty())
                        m_goboSlotNames.push_back(slot.name);
                }
            }
        }
        else
        {
            // Fallback to default gobo
            m_goboTexture->LoadFromFile(device, "data/models/gobo.jpg");
            m_goboSlotNames.push_back("Default");
        }
    }
    else
    {
        // No GDTF gobos, use default
        if (!m_goboTexture->LoadFromFile(device, "data/models/gobo.jpg"))
        {
            m_goboTexture->LoadFromFile(device, "data/models/stage.png");
        }
        m_goboSlotNames.push_back("Default");
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

        // Default gobo (can be changed via UI)
        light.SetGoboIndex(3);

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
                orientationNode->setRotation(DirectX::XM_PIDIV2, 0.0f, 0.0f);
                // Scale 2x
                orientationNode->setScale(2.0f, 2.0f, 2.0f);
                
                // Bring it up a little to touch the truss
                placementNode->setTranslation(pos.x, pos.y + 0.45f, pos.z);
                
                                placementNode->addChild(orientationNode);
                                orientationNode->addChild(instanceRoot);
                
                                // Link spotlight to nodes for animation
                                auto panNode = instanceRoot->findChild("Yoke");
                                auto tiltNode = instanceRoot->findChild("Head");
                                auto beamNode = instanceRoot->findChild("Beam");
                
                                // Fallback: search for nodes containing the names if exact match fails
                                if (!panNode) panNode = instanceRoot->findChild("Pan");
                                if (!tiltNode) tiltNode = instanceRoot->findChild("Tilt");
                
                                m_spotlights.back().LinkNodes(panNode, tiltNode, beamNode);
                
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

    // Demo Mode Logic
    if (m_demoMode)
    {
        for (size_t i = 0; i < m_spotlights.size(); ++i)
        {
            auto &light = m_spotlights[i];
            float phase = static_cast<float>(i) * 0.5f;
            
            // Phased Sine Pan/Tilt
            float pan = std::sin(m_time * 0.8f + phase) * 45.0f;
            float tilt = std::cos(m_time * 1.2f + phase) * 30.0f - 20.0f;
            light.SetPan(pan);
            light.SetTilt(tilt);

            // Rainbow color chase
            float hue = fmodf(m_time * 0.2f + static_cast<float>(i) * 0.25f, 1.0f);
            
            // HSV to RGB (simplified)
            float r = 0, g = 0, b = 0;
            float h = hue * 6.0f;
            float x = 1.0f - std::abs(fmodf(h, 2.0f) - 1.0f);
            if (h < 1.0f) { r = 1; g = x; }
            else if (h < 2.0f) { r = x; g = 1; }
            else if (h < 3.0f) { g = 1; b = x; }
            else if (h < 4.0f) { g = x; b = 1; }
            else if (h < 5.0f) { r = x; b = 1; }
            else { r = 1; b = x; }
            
            light.SetColor(r, g, b);

            // Add smooth gobo rotation
            light.SetGoboRotation(m_time * 0.5f + phase);
        }
    }

    // Sync spotlights with their respective nodes
    for (auto &light : m_spotlights)
    {
        light.UpdateFromNodes();
        // light.UpdateLightMatrix(); // Removed: UpdateFromNodes now handles matrix smoothly
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
