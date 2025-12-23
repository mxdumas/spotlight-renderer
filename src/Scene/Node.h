#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

namespace SceneGraph {

/**
 * @class Node
 * @brief A node in the hierarchical scene graph.
 * 
 * Each node has a local transform and computes its world transform based on its parent.
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    Node(const std::string& name = "Node");
    virtual ~Node() = default;

    /**
     * @brief Adds a child node.
     */
    void AddChild(std::shared_ptr<Node> child);

    /**
     * @brief Updates world transforms for this node and all its children.
     * @param parentWorld The world matrix of the parent node.
     */
    void UpdateWorldMatrix(const DirectX::XMMATRIX& parentWorld = DirectX::XMMatrixIdentity());

    // Accessors
    const std::string& GetName() const { return m_name; }
    const DirectX::XMMATRIX& GetWorldMatrix() const { return m_worldMatrix; }
    const DirectX::XMMATRIX& GetLocalMatrix() const { return m_localMatrix; }

    // Transform modifiers
    void SetTranslation(float x, float y, float z);
    void SetRotation(float roll, float pitch, float yaw);
    void SetScale(float x, float y, float z);
    void SetLocalMatrix(const DirectX::XMMATRIX& matrix) { m_localMatrix = matrix; }

protected:
    std::string m_name;
    
    DirectX::XMMATRIX m_localMatrix;
    DirectX::XMMATRIX m_worldMatrix;

    DirectX::XMFLOAT3 m_translation = {0,0,0};
    DirectX::XMFLOAT3 m_rotation = {0,0,0};
    DirectX::XMFLOAT3 m_scale = {1,1,1};

    std::weak_ptr<Node> m_parent;
    std::vector<std::shared_ptr<Node>> m_children;
};

} // namespace SceneGraph
