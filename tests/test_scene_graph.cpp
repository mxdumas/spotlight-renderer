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
    node->SetTranslation(1.0f, 2.0f, 3.0f);
    node->UpdateWorldMatrix();

    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, node->GetWorldMatrix());

    assert(NearEqual(world._41, 1.0f));
    assert(NearEqual(world._42, 2.0f));
    assert(NearEqual(world._43, 3.0f));
    std::cout << "Simple transform passed." << std::endl;
}

void TestHierarchyTransform() {
    std::cout << "Testing hierarchy transform..." << std::endl;
    auto parent = std::make_shared<SceneGraph::Node>("Parent");
    parent->SetTranslation(10.0f, 0.0f, 0.0f);

    auto child = std::make_shared<SceneGraph::Node>("Child");
    child->SetTranslation(5.0f, 0.0f, 0.0f);
    parent->AddChild(child);

    parent->UpdateWorldMatrix();

    DirectX::XMFLOAT4X4 parentWorld;
    DirectX::XMStoreFloat4x4(&parentWorld, parent->GetWorldMatrix());
    assert(NearEqual(parentWorld._41, 10.0f));

    DirectX::XMFLOAT4X4 childWorld;
    DirectX::XMStoreFloat4x4(&childWorld, child->GetWorldMatrix());
    // Child should be at 10 + 5 = 15
    assert(NearEqual(childWorld._41, 15.0f));
    std::cout << "Hierarchy transform passed." << std::endl;
}

void TestRotationPropagation() {
    std::cout << "Testing rotation propagation..." << std::endl;
    auto parent = std::make_shared<SceneGraph::Node>("Parent");
    // Rotate parent 90 degrees around Y axis
    parent->SetRotation(0.0f, 0.0f, DirectX::XM_PIDIV2);

    auto child = std::make_shared<SceneGraph::Node>("Child");
    // Child is 1 unit forward in its local Z
    child->SetTranslation(0.0f, 0.0f, 1.0f);
    parent->AddChild(child);

    parent->UpdateWorldMatrix();

    DirectX::XMFLOAT4X4 childWorld;
    DirectX::XMStoreFloat4x4(&childWorld, child->GetWorldMatrix());
    
    // After 90 deg rotation around Y, local Z(0,0,1) becomes world X(1,0,0)
    assert(NearEqual(childWorld._41, 1.0f));
    assert(NearEqual(childWorld._43, 0.0f));
    std::cout << "Rotation propagation passed." << std::endl;
}

void TestFindChild() {
    std::cout << "Testing FindChild..." << std::endl;
    auto root = std::make_shared<SceneGraph::Node>("Root");
    auto child1 = std::make_shared<SceneGraph::Node>("Child1");
    auto child2 = std::make_shared<SceneGraph::Node>("Child2");
    auto grandchild = std::make_shared<SceneGraph::Node>("Grandchild");

    root->AddChild(child1);
    root->AddChild(child2);
    child1->AddChild(grandchild);

    assert(root->FindChild("Root") == root);
    assert(root->FindChild("Child1") == child1);
    assert(root->FindChild("Child2") == child2);
    assert(root->FindChild("Grandchild") == grandchild);
    assert(root->FindChild("NonExistent") == nullptr);
    std::cout << "FindChild passed." << std::endl;
}

int main() {
    try {
        TestSimpleTransform();
        TestHierarchyTransform();
        TestRotationPropagation();
        TestFindChild();
        std::cout << "All SceneGraph tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
