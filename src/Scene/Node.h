#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

namespace SceneGraph
{

/**
 * @class Node
 * @brief A node in the hierarchical scene graph.
 * 
 * Each node has a local transform and computes its world transform based on its parent.
 * It manages a list of child nodes and propagates transform updates down the tree.
 */
class Node : public std::enable_shared_from_this<Node>
{
public:
    /**
     * @brief Constructs a new Node.
     * @param name The debug name for this node.
     */
    Node(const std::string& name = "Node");

    /**
     * @brief Virtual destructor for inheritance.
     */
    virtual ~Node() = default;

    /**
     * @brief Adds a child node to this node.
     * @param child A shared pointer to the node to add as a child.
     */
    void addChild(std::shared_ptr<Node> child);

    /**
     * @brief Updates the world transform for this node and recursively for all its children.
     * @param parent_world The world matrix of the parent node (defaults to identity).
     */
    void updateWorldMatrix(const DirectX::XMMATRIX& parent_world = DirectX::XMMatrixIdentity());

    /**
     * @brief Gets the debug name of the node.
     * @return Const reference to the name string.
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Gets the computed world transformation matrix.
     * @return Const reference to the world XMMATRIX.
     */
    const DirectX::XMMATRIX& getWorldMatrix() const { return world_matrix_; }

    /**
     * @brief Gets the local transformation matrix relative to the parent.
     * @return Const reference to the local XMMATRIX.
     */
    const DirectX::XMMATRIX& getLocalMatrix() const { return local_matrix_; }

    /**
     * @brief Sets the local translation components.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     */
    void setTranslation(float x, float y, float z);

    /**
     * @brief Sets the local rotation using Euler angles.
     * @param roll Rotation around Z axis.
     * @param pitch Rotation around X axis.
     * @param yaw Rotation around Y axis.
     */
    void setRotation(float roll, float pitch, float yaw);

    /**
     * @brief Sets the local scale components.
     * @param x Scale factor for X.
     * @param y Scale factor for Y.
     * @param z Scale factor for Z.
     */
    void setScale(float x, float y, float z);

    /**
     * @brief Directly sets the local transformation matrix.
     * @param matrix The new local XMMATRIX.
     */
    void setLocalMatrix(const DirectX::XMMATRIX& matrix) { local_matrix_ = matrix; }

protected:
    std::string name_; ///< Debug name of the node.
    
    DirectX::XMMATRIX local_matrix_; ///< Relative transform to parent.
    DirectX::XMMATRIX world_matrix_; ///< Absolute transform in world space.

    DirectX::XMFLOAT3 translation_ = {0.0f, 0.0f, 0.0f}; ///< Local translation vector.
    DirectX::XMFLOAT3 rotation_ = {0.0f, 0.0f, 0.0f};    ///< Local Euler angles.
    DirectX::XMFLOAT3 scale_ = {1.0f, 1.0f, 1.0f};       ///< Local scale vector.

    std::weak_ptr<Node> parent_;                      ///< Weak pointer to parent to avoid cycles.
    std::vector<std::shared_ptr<Node>> children_;     ///< List of owned child nodes.
};

} // namespace SceneGraph
