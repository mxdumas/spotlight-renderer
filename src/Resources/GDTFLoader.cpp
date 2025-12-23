#include "GDTFLoader.h"
#include "../Scene/MeshNode.h"

namespace GDTF
{

std::shared_ptr<SceneGraph::Node> GDTFLoader::buildSceneGraph(const GDTFParser &parser)
{
    auto gdtf_root = parser.getGeometryRoot();
    if (!gdtf_root)
    {
        return nullptr;
    }

    return createNodeRecursive(gdtf_root);
}

std::shared_ptr<SceneGraph::Node> GDTFLoader::createNodeRecursive(std::shared_ptr<GeometryNode> gdtf_node)
{
    if (!gdtf_node)
    {
        return nullptr;
    }

    // Create a generic node for now
    // In Phase 3, we will link this to actual GLB meshes
    auto scene_node = std::make_shared<SceneGraph::Node>(gdtf_node->name);

    scene_node->setTranslation(gdtf_node->position.x, gdtf_node->position.y, gdtf_node->position.z);
    scene_node->setRotation(gdtf_node->rotation.x, gdtf_node->rotation.y, gdtf_node->rotation.z);

    for (auto &child_gdtf : gdtf_node->children)
    {
        auto child_scene = createNodeRecursive(child_gdtf);
        if (child_scene)
        {
            scene_node->addChild(child_scene);
        }
    }

    return scene_node;
}

} // namespace GDTF
