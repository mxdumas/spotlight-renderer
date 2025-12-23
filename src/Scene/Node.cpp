#include "Node.h"

namespace SceneGraph {

Node::Node(const std::string& name)
    : m_name(name)
{
    m_localMatrix = DirectX::XMMatrixIdentity();
    m_worldMatrix = DirectX::XMMatrixIdentity();
}

void Node::AddChild(std::shared_ptr<Node> child) {
    if (child) {
        child->m_parent = shared_from_this();
        m_children.push_back(child);
    }
}

void Node::UpdateWorldMatrix(const DirectX::XMMATRIX& parentWorld) {
    // Recompute local matrix from T, R, S if needed
    // For now, assume m_localMatrix is updated or computed on the fly
    m_localMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z) *
                    DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z) *
                    DirectX::XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);

    m_worldMatrix = m_localMatrix * parentWorld;

    for (auto& child : m_children) {
        child->UpdateWorldMatrix(m_worldMatrix);
    }
}

void Node::SetTranslation(float x, float y, float z) {
    m_translation = {x, y, z};
}

void Node::SetRotation(float roll, float pitch, float yaw) {
    m_rotation = {roll, pitch, yaw};
}

void Node::SetScale(float x, float y, float z) {
    m_scale = {x, y, z};
}

} // namespace SceneGraph
