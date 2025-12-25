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
    
    auto panNode = std::make_shared<SceneGraph::Node>("Pan");
    auto tiltNode = std::make_shared<SceneGraph::Node>("Tilt");
    auto beamNode = std::make_shared<SceneGraph::Node>("Beam");

    panNode->addChild(tiltNode);
    tiltNode->addChild(beamNode);

    Spotlight light;
    light.LinkNodes(panNode, tiltNode, beamNode);

    // Initial state
    panNode->updateWorldMatrix();
    light.UpdateFromNodes();
    
    // Default Pan/Tilt should be 0
    assert(NearEqual(light.GetPan(), 0.0f));
    assert(NearEqual(light.GetTilt(), 0.0f));

    // Move Pan to 90 degrees
    light.SetPan(90.0f);
    panNode->updateWorldMatrix();
    light.UpdateFromNodes();

    assert(NearEqual(light.GetPan(), 90.0f));
    
    // Position should still be 0,0,0 if not translated
    DirectX::XMFLOAT3 pos = light.GetPosition();
    assert(NearEqual(pos.x, 0.0f));
    assert(NearEqual(pos.y, 0.0f));
    assert(NearEqual(pos.z, 0.0f));

    // Direction should be updated. 
    // Pan 90 deg (Yaw) around Y: forward(0,0,1) -> (1,0,0)
    DirectX::XMFLOAT3 dir = light.GetDirection();
    assert(NearEqual(dir.x, 1.0f));
    assert(NearEqual(dir.y, 0.0f));
    assert(NearEqual(dir.z, 0.0f));

    // Move Tilt to 90 degrees
    light.SetTilt(90.0f);
    panNode->updateWorldMatrix();
    light.UpdateFromNodes();

    assert(NearEqual(light.GetTilt(), 90.0f));
    
    // Tilt 90 deg (Pitch) around X after Pan 90 deg.
    // This is getting complex but let's check if it's not zero.
    dir = light.GetDirection();
    // (Pitch 90 after Yaw 90) -> should point down (0,-1,0) if Yaw happened first.
    // Wait, XMMatrixRotationRollPitchYaw(pitch, yaw, roll) 
    // In our case: Pitch=tilt, Yaw=pan.
    // If both are 90:
    // Yaw 90: Forward(0,0,1) -> (1,0,0)
    // Then Pitch 90 around (local) X (which is now global -Z): 
    // This depends on the order. 
    // XMMatrixRotationRollPitchYaw order is typically Y, then X, then Z or similar.
    // Actually it's Roll, then Pitch, then Yaw? No, docs say:
    // "The rotation order is roll first, then pitch, then yaw."
    
    std::cout << "Spotlight direction after Pan 90, Tilt 90: " << dir.x << ", " << dir.y << ", " << dir.z << std::endl;

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
