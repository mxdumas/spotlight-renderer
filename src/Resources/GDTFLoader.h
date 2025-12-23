#pragma once

#include <map>
#include <memory>
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
     * @param parser A reference to a GDTFParser that has already successfully loaded a file.
     * @return A shared pointer to the root Node of the generated SceneGraph.
     */
    std::shared_ptr<SceneGraph::Node> buildSceneGraph(const GDTFParser &parser);

private:
    /**
     * @brief Recursively converts GDTF GeometryNodes into SceneGraph Nodes.
     *
     * @param gdtf_node The current GDTF logical node.
     * @return A shared pointer to the created SceneGraph Node.
     */
    std::shared_ptr<SceneGraph::Node> createNodeRecursive(std::shared_ptr<GeometryNode> gdtf_node);
};

} // namespace GDTF
