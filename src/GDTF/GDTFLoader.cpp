/**
 * @file GDTFLoader.cpp
 * @brief Implementation of scene graph construction from parsed GDTF data.
 */

#include "GDTFLoader.h"
#include "../Scene/MeshNode.h"
#include "ModelLoader.h"
#include "../Geometry/GeometryGenerator.h"
#include <fstream>
#include <iostream>

namespace GDTF
{

std::shared_ptr<SceneGraph::Node> GDTFLoader::buildSceneGraph(ID3D11Device *device, GDTFParser &parser)
{
    auto gdtf_root = parser.getGeometryRoot();
    if (!gdtf_root)
    {
        return nullptr;
    }

    m_meshCache.clear();
    auto root = createNodeRecursive(device, parser, gdtf_root);
    
    return root;
}

std::shared_ptr<SceneGraph::Node> GDTFLoader::createNodeRecursive(ID3D11Device *device, GDTFParser &parser,
                                                                  std::shared_ptr<GeometryNode> gdtf_node)
{
    if (!gdtf_node)
    {
        return nullptr;
    }

    std::shared_ptr<SceneGraph::Node> scene_node;

    // Check if this node has a model
    if (!gdtf_node->model.empty())
    {
        std::string model_file = parser.getModelFile(gdtf_node->model);
        std::shared_ptr<Mesh> mesh;
        
        if (m_meshCache.count(model_file))
        {
            mesh = m_meshCache[model_file];
        }
        else
        {
            // Try to load model from GDTF
            std::vector<uint8_t> model_data;
            std::string model_path = model_file;
            
            // Only try to load if it's a known 3D format we support (GLB/GLTF/3DS) or no extension
            bool is_supported = (model_path.find(".glb") != std::string::npos || 
                                 model_path.find(".gltf") != std::string::npos || 
                                 model_path.find(".3ds") != std::string::npos ||
                                 model_path.find(".") == std::string::npos);
            
            if (is_supported)
            {
                // Try various search patterns
                std::vector<std::string> search_paths;
                std::string base_name = model_path;
                
                // If no extension, try .glb and .3ds
                if (base_name.find(".") == std::string::npos) {
                    search_paths.push_back(base_name + ".glb");
                    search_paths.push_back("models/" + base_name + ".glb");
                    search_paths.push_back(base_name + ".3ds");
                    search_paths.push_back("models/" + base_name + ".3ds");
                    search_paths.push_back("models/3ds/" + base_name + ".3ds");
                } else {
                    search_paths.push_back(base_name);
                    search_paths.push_back("models/" + base_name);
                    if (base_name.find(".3ds") != std::string::npos) {
                        search_paths.push_back("models/3ds/" + base_name);
                    }
                }

                for (const auto& path : search_paths) {
                    if (parser.extractFile(path, model_data)) {
                        model_path = path; // Update hint for Assimp
                        break;
                    }
                }
            }

            if (!model_data.empty())
            {
                mesh = ModelLoader::LoadFromMemory(device, model_data.data(), model_data.size(), model_path);
            }
            
            if (mesh) {
                m_meshCache[model_file] = mesh;
                std::ofstream log("debug.log", std::ios::app);
                log << "Loaded model mesh: " << model_file << " with Assimp." << std::endl;
            }
        }

        if (mesh)
        {
            scene_node = std::make_shared<SceneGraph::MeshNode>(mesh, gdtf_node->name);
        }
    }

    if (!scene_node)
    {
        scene_node = std::make_shared<SceneGraph::Node>(gdtf_node->name);
    }

    // Set local matrix from GDTF
    scene_node->setLocalMatrix(DirectX::XMLoadFloat4x4(&gdtf_node->matrix));

    for (auto &child_gdtf : gdtf_node->children)
    {
        auto child_scene = createNodeRecursive(device, parser, child_gdtf);
        if (child_scene)
        {
            scene_node->addChild(child_scene);
        }
    }

    return scene_node;
}

} // namespace GDTF