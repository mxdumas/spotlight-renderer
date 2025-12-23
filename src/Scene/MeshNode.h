#pragma once

#include "Node.h"
#include "../Resources/Mesh.h"

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
    MeshNode(std::shared_ptr<Mesh> mesh, const std::string& name = "MeshNode")
        : Node(name), mesh_(mesh) {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~MeshNode() = default;

    /**
     * @brief Gets the associated mesh resource.
     * @return A shared pointer to the Mesh.
     */
    std::shared_ptr<Mesh> getMesh() const { return mesh_; }

    /**
     * @brief Sets or replaces the mesh resource for this node.
     * @param mesh A shared pointer to the new Mesh.
     */
    void setMesh(std::shared_ptr<Mesh> mesh) { mesh_ = mesh; }

private:
    std::shared_ptr<Mesh> mesh_; ///< The Mesh resource to be rendered at this node's position.
};

} // namespace SceneGraph
