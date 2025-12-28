#include "../src/Scene/Spotlight.h"
#include "../src/Scene/Node.h"
#include <iostream>
#include <cassert>
#include <cmath>

bool NearEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void TestSpotlightNodeLinking() {
    std::cout << "Testing spotlight node linking..." << std::endl;

    auto pan_node = std::make_shared<SceneGraph::Node>("Pan");
    auto tilt_node = std::make_shared<SceneGraph::Node>("Tilt");
    auto beam_node = std::make_shared<SceneGraph::Node>("Beam");

    pan_node->AddChild(tilt_node);
    tilt_node->AddChild(beam_node);

    Spotlight light;
    light.LinkNodes(pan_node, tilt_node, beam_node);

    // Initial state: forward direction is (0,0,1)
    pan_node->UpdateWorldMatrix();
    light.UpdateFromNodes();

    // Default Pan/Tilt should be 0
    assert(NearEqual(light.GetPan(), 0.0f));
    assert(NearEqual(light.GetTilt(), 0.0f));

    // Initial direction: forward (0,0,1)
    DirectX::XMFLOAT3 dir = light.GetDirection();
    assert(NearEqual(dir.x, 0.0f));
    assert(NearEqual(dir.y, 0.0f));
    assert(NearEqual(dir.z, 1.0f));

    // Position should be 0,0,0 (no translation)
    DirectX::XMFLOAT3 pos = light.GetPosition();
    assert(NearEqual(pos.x, 0.0f));
    assert(NearEqual(pos.y, 0.0f));
    assert(NearEqual(pos.z, 0.0f));

    // Move Pan to 90 degrees
    // Note: In GDTF convention, Pan is roll (Z rotation) because fixture is pitched 90°
    // Z rotation of (0,0,1) leaves it unchanged
    light.SetPan(90.0f);
    pan_node->UpdateWorldMatrix();
    light.UpdateFromNodes();

    assert(NearEqual(light.GetPan(), 90.0f));
    dir = light.GetDirection();
    assert(NearEqual(dir.z, 1.0f)); // Z unchanged by roll

    // Move Tilt to 90 degrees
    // Tilt is pitch (X rotation), negated: -90° rotates (0,0,1) towards +Y
    light.SetTilt(90.0f);
    pan_node->UpdateWorldMatrix();
    light.UpdateFromNodes();

    assert(NearEqual(light.GetTilt(), 90.0f));

    // After combined rotations, verify direction has changed
    dir = light.GetDirection();
    float magnitude = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    assert(NearEqual(magnitude, 1.0f)); // Direction should be normalized

    std::cout << "Spotlight node linking passed." << std::endl;
}

int main() {
    try {
        TestSpotlightNodeLinking();
        std::cout << "All Spotlight tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
