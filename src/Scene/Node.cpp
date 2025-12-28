/**
 * @file Node.cpp
 * @brief Implementation of hierarchical scene graph node transforms.
 */

#include "Node.h"

namespace SceneGraph
{

Node::Node(const std::string &name) : m_name(name)
{
    m_baseMatrix = DirectX::XMMatrixIdentity();
    m_localMatrix = DirectX::XMMatrixIdentity();
    m_worldMatrix = DirectX::XMMatrixIdentity();
}

void Node::AddChild(std::shared_ptr<Node> child)
{
    if (child)
    {
        child->m_parent = shared_from_this();
        m_children.push_back(child);
    }
}

std::shared_ptr<Node> Node::FindChild(const std::string &name)
{
    if (m_name == name)
    {
        return shared_from_this();
    }

    for (auto &child : m_children)
    {
        auto found = child->FindChild(name);
        if (found)
        {
            return found;
        }
    }

    return nullptr;
}

void Node::UpdateWorldMatrix(const DirectX::XMMATRIX &parent_world)
{
    if (m_hasBaseMatrix)
    {
        // GDTF mode: combine animation rotation with base matrix
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x,  // pitch (X axis) - for tilt
                                                                           m_rotation.y,  // yaw (Y axis) - for pan
                                                                           m_rotation.z); // roll (Z axis)
        m_localMatrix = rotation * m_baseMatrix;
    }
    else if (m_useComponents)
    {
        // Wrapper mode: use T/R/S components
        m_localMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z) *
                        DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z) *
                        DirectX::XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);
    }
    // else: m_localMatrix stays as identity from constructor

    m_worldMatrix = m_localMatrix * parent_world;

    for (auto &child : m_children)
    {
        child->UpdateWorldMatrix(m_worldMatrix);
    }
}

void Node::SetTranslation(float x, float y, float z)
{
    m_translation = {x, y, z};
    if (!m_hasBaseMatrix)
        m_useComponents = true;
}

void Node::SetRotation(float pitch, float yaw, float roll)
{
    // pitch = X axis (tilt), yaw = Y axis (pan), roll = Z axis
    m_rotation = {pitch, yaw, roll};
    if (!m_hasBaseMatrix)
        m_useComponents = true;
}

void Node::SetScale(float x, float y, float z)
{
    m_scale = {x, y, z};
    if (!m_hasBaseMatrix)
        m_useComponents = true;
}

} // namespace SceneGraph
