#include "Node.h"

namespace SceneGraph
{

Node::Node(const std::string &name) : name_(name)
{
    local_matrix_ = DirectX::XMMatrixIdentity();
    world_matrix_ = DirectX::XMMatrixIdentity();
}

void Node::addChild(std::shared_ptr<Node> child)
{
    if (child)
    {
        child->parent_ = shared_from_this();
        children_.push_back(child);
    }
}

void Node::updateWorldMatrix(const DirectX::XMMATRIX &parent_world)
{
    // Recompute local matrix from T, R, S components
    local_matrix_ = DirectX::XMMatrixScaling(scale_.x, scale_.y, scale_.z) *
                    DirectX::XMMatrixRotationRollPitchYaw(rotation_.x, rotation_.y, rotation_.z) *
                    DirectX::XMMatrixTranslation(translation_.x, translation_.y, translation_.z);

    world_matrix_ = local_matrix_ * parent_world;

    for (auto &child : children_)
    {
        child->updateWorldMatrix(world_matrix_);
    }
}

void Node::setTranslation(float x, float y, float z)
{
    translation_ = {x, y, z};
}

void Node::setRotation(float roll, float pitch, float yaw)
{
    rotation_ = {roll, pitch, yaw};
}

void Node::setScale(float x, float y, float z)
{
    scale_ = {x, y, z};
}

} // namespace SceneGraph
