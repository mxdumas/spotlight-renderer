/**
 * @file MeshNode.h
 * @brief Scene graph node that holds a renderable mesh resource.
 */

#pragma once

#include "../Resources/Mesh.h"
#include "Node.h"

namespace SceneGraph
{

/**
 * @class MeshNode
 * @brief A scene graph node that represents a drawable mesh.
 *
 * This node links the hierarchy to a specific Mesh resource and allows
 * rendering it using the node's computed world matrix.
 */
class MeshNode : public Node
{
public:
    /**
     * @brief Constructs a new MeshNode.
     * @param mesh A shared pointer to the Mesh resource.
     * @param name The debug name for this node.
     */
    MeshNode(std::shared_ptr<Mesh> mesh, const std::string &name = "MeshNode") : Node(name), m_mesh(mesh)
    {
    }

    /**
     * @brief Virtual destructor.
     */
    ~MeshNode() override = default;

    /**
     * @brief Gets the associated mesh resource.
     * @return A shared pointer to the Mesh.
     */
    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const
    {
        return m_mesh;
    }

    /**
     * @brief Sets or replaces the mesh resource for this node.
     * @param mesh A shared pointer to the new Mesh.
     */
    void SetMesh(std::shared_ptr<Mesh> mesh)
    {
        m_mesh = mesh;
    }

private:
    std::shared_ptr<Mesh> m_mesh; ///< The Mesh resource to be rendered at this node's position.
};

} // namespace SceneGraph
