/**
 * @file GDTFLoader.h
 * @brief Scene graph construction from parsed GDTF fixture data.
 */

#pragma once

#include <d3d11.h>
#include <map>
#include <memory>
#include "../Resources/Mesh.h"
#include "../Scene/Node.h"
#include "GDTFParser.h"

namespace GDTF
{

/**
 * @class GDTFLoader
 * @brief Orchestrates the creation of a scene graph from parsed GDTF data.
 *
 * This class takes the logical tree from GDTFParser and converts it into
 * a hierarchy of SceneGraph::Node objects, mapping geometry types to specialized nodes.
 */
class GDTFLoader
{
public:
    /**
     * @brief Default constructor.
     */
    GDTFLoader() = default;

    /**
     * @brief Default destructor.
     */
    ~GDTFLoader() = default;

    /**
     * @brief Builds a SceneGraph hierarchy from a parsed GDTF.
     *
     * @param device Pointer to the ID3D11Device for mesh creation.
     * @param parser A reference to a GDTFParser that has already successfully loaded a file.
     * @return A shared pointer to the root Node of the generated SceneGraph.
     */
    static std::shared_ptr<SceneGraph::Node> BuildSceneGraph(ID3D11Device *device, GDTFParser &parser);

private:
    /**
     * @brief Recursively converts GDTF GeometryNodes into SceneGraph Nodes.
     *
     * @param device Pointer to the ID3D11Device.
     * @param parser Reference to the GDTFParser.
     * @param gdtf_node The current GDTF logical node.
     * @param mesh_cache Cache of loaded meshes.
     * @return A shared pointer to the created SceneGraph Node.
     */
    static std::shared_ptr<SceneGraph::Node>
    CreateNodeRecursive(ID3D11Device *device, GDTFParser &parser, std::shared_ptr<GeometryNode> gdtf_node,
                        std::map<std::string, std::shared_ptr<Mesh>> &mesh_cache);
};

} // namespace GDTF
