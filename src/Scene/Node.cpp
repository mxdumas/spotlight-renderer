/**
 * @file Node.cpp
 * @brief Implementation of hierarchical scene graph node transforms.
 */

#include "Node.h"

namespace SceneGraph
{

Node::Node(const std::string &name) : m_name(name)
{
    m_localMatrix = DirectX::XMMatrixIdentity();
    m_worldMatrix = DirectX::XMMatrixIdentity();
}

void Node::addChild(std::shared_ptr<Node> child)
{
    if (child)
    {
        child->m_parent = shared_from_this();
        m_children.push_back(child);
    }
}

void Node::updateWorldMatrix(const DirectX::XMMATRIX &parent_world)
{
    // Recompute local matrix from T, R, S components if enabled
    if (m_useComponents)
    {
        // XMMatrixRotationRollPitchYaw takes (pitch, yaw, roll)
        m_localMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z) *
                        DirectX::XMMatrixRotationRollPitchYaw(m_rotation.y, m_rotation.z, m_rotation.x) *
                        DirectX::XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);
    }

    m_worldMatrix = m_localMatrix * parent_world;

    for (auto &child : m_children)
    {
        child->updateWorldMatrix(m_worldMatrix);
    }
}

void Node::setTranslation(float x, float y, float z)
{
    m_translation = {x, y, z};
    m_useComponents = true;
}

void Node::setRotation(float roll, float pitch, float yaw)
{
    m_rotation = {roll, pitch, yaw};
    m_useComponents = true;
}

void Node::setScale(float x, float y, float z)
{
    m_scale = {x, y, z};
    m_useComponents = true;
}

} // namespace SceneGraph
