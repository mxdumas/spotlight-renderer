#include "../src/Scene/Node.h"
#include <iostream>
#include <cassert>
#include <cmath>

bool NearEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void TestSimpleTransform() {
    std::cout << "Testing simple transform..." << std::endl;
    auto node = std::make_shared<SceneGraph::Node>("TestNode");
    node->setTranslation(1.0f, 2.0f, 3.0f);
    node->updateWorldMatrix();

    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, node->getWorldMatrix());

    assert(NearEqual(world._41, 1.0f));
    assert(NearEqual(world._42, 2.0f));
    assert(NearEqual(world._43, 3.0f));
    std::cout << "Simple transform passed." << std::endl;
}

void TestHierarchyTransform() {
    std::cout << "Testing hierarchy transform..." << std::endl;
    auto parent = std::make_shared<SceneGraph::Node>("Parent");
    parent->setTranslation(10.0f, 0.0f, 0.0f);

    auto child = std::make_shared<SceneGraph::Node>("Child");
    child->setTranslation(5.0f, 0.0f, 0.0f);
    parent->addChild(child);

    parent->updateWorldMatrix();

    DirectX::XMFLOAT4X4 parentWorld;
    DirectX::XMStoreFloat4x4(&parentWorld, parent->getWorldMatrix());
    assert(NearEqual(parentWorld._41, 10.0f));

    DirectX::XMFLOAT4X4 childWorld;
    DirectX::XMStoreFloat4x4(&childWorld, child->getWorldMatrix());
    // Child should be at 10 + 5 = 15
    assert(NearEqual(childWorld._41, 15.0f));
    std::cout << "Hierarchy transform passed." << std::endl;
}

void TestRotationPropagation() {
    std::cout << "Testing rotation propagation..." << std::endl;
    auto parent = std::make_shared<SceneGraph::Node>("Parent");
    // Rotate parent 90 degrees around Y axis
    parent->setRotation(0.0f, 0.0f, DirectX::XM_PIDIV2); 

    auto child = std::make_shared<SceneGraph::Node>("Child");
    // Child is 1 unit forward in its local Z
    child->setTranslation(0.0f, 0.0f, 1.0f);
    parent->addChild(child);

    parent->updateWorldMatrix();

    DirectX::XMFLOAT4X4 childWorld;
    DirectX::XMStoreFloat4x4(&childWorld, child->getWorldMatrix());
    
    // After 90 deg rotation around Y, local Z(0,0,1) becomes world X(1,0,0)
    assert(NearEqual(childWorld._41, 1.0f));
    assert(NearEqual(childWorld._43, 0.0f));
    std::cout << "Rotation propagation passed." << std::endl;
}

int main() {
    try {
        TestSimpleTransform();
        TestHierarchyTransform();
        TestRotationPropagation();
        std::cout << "All SceneGraph tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
