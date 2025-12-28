/**
 * @file Node.h
 * @brief Base node class for the hierarchical scene graph.
 */

#pragma once

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

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
    explicit Node(std::string name = "Node");

    /**
     * @brief Virtual destructor for inheritance.
     */
    virtual ~Node() = default;

    /**
     * @brief Adds a child node to this node.
     * @param child A shared pointer to the node to add as a child.
     */
    void AddChild(const std::shared_ptr<Node> &child);

    /**
     * @brief Updates the world transform for this node and recursively for all its children.
     * @param parentWorld The world matrix of the parent node (defaults to identity).
     */
    void UpdateWorldMatrix(const DirectX::XMMATRIX &parentWorld = DirectX::XMMatrixIdentity());

    /**
     * @brief Gets the list of child nodes.
     * @return Const reference to the vector of child shared pointers.
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Node>> &GetChildren() const
    {
        return m_children;
    }

    /**
     * @brief Finds a child node by name recursively.
     * @param name The name of the node to find.
     * @return A shared pointer to the found node, or nullptr.
     */
    std::shared_ptr<Node> FindChild(const std::string &name);

    /**
     * @brief Gets the debug name of the node.
     * @return Const reference to the name string.
     */
    [[nodiscard]] const std::string &GetName() const
    {
        return m_name;
    }

    /**
     * @brief Gets the computed world transformation matrix.
     * @return Const reference to the world XMMATRIX.
     */
    [[nodiscard]] const DirectX::XMMATRIX &GetWorldMatrix() const
    {
        return m_worldMatrix;
    }

    /**
     * @brief Gets the local transformation matrix relative to the parent.
     * @return Const reference to the local XMMATRIX.
     */
    [[nodiscard]] const DirectX::XMMATRIX &GetLocalMatrix() const
    {
        return m_localMatrix;
    }

    /**
     * @brief Sets the local translation components.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param z Z coordinate.
     */
    void SetTranslation(float x, float y, float z);

    /**
     * @brief Sets the animation rotation (combined with base matrix).
     * @param pitch Rotation around X axis (tilt).
     * @param yaw Rotation around Y axis (pan).
     * @param roll Rotation around Z axis.
     */
    void SetRotation(float pitch, float yaw, float roll);

    /**
     * @brief Sets the local scale components.
     * @param x Scale factor for X.
     * @param y Scale factor for Y.
     * @param z Scale factor for Z.
     */
    void SetScale(float x, float y, float z);

    /**
     * @brief Sets the base transformation matrix (e.g., GDTF placement).
     *        Rotation can still be applied on top via SetRotation().
     * @param matrix The base XMMATRIX.
     */
    void SetLocalMatrix(const DirectX::XMMATRIX &matrix)
    {
        m_baseMatrix = matrix;
        m_hasBaseMatrix = true;
    }

protected:
    std::string m_name; ///< Debug name of the node.

    DirectX::XMMATRIX m_baseMatrix;  ///< Base transform from GDTF placement.
    DirectX::XMMATRIX m_localMatrix; ///< Final local transform.
    DirectX::XMMATRIX m_worldMatrix; ///< Absolute transform in world space.

    bool m_hasBaseMatrix = false;                         ///< Whether base matrix was set (GDTF mode).
    bool m_useComponents = false;                         ///< Whether T/R/S components are used.
    DirectX::XMFLOAT3 m_translation = {0.0f, 0.0f, 0.0f}; ///< Translation for wrapper nodes.
    DirectX::XMFLOAT3 m_rotation = {0.0f, 0.0f, 0.0f};    ///< Rotation (animation or wrapper).
    DirectX::XMFLOAT3 m_scale = {1.0f, 1.0f, 1.0f};       ///< Scale for wrapper nodes.

    std::weak_ptr<Node> m_parent;                  ///< Weak pointer to parent to avoid cycles.
    std::vector<std::shared_ptr<Node>> m_children; ///< List of owned child nodes.
};

} // namespace SceneGraph
