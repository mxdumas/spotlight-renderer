/**
 * @file GDTFLoader.cpp
 * @brief Implementation of scene graph construction from parsed GDTF data.
 */

#include "GDTFLoader.h"
#include <fstream>
#include <iostream>
#include "../Geometry/GeometryGenerator.h"
#include "../Scene/MeshNode.h"
#include "ModelLoader.h"

namespace GDTF
{

std::shared_ptr<SceneGraph::Node> GDTFLoader::BuildSceneGraph(ID3D11Device *device, GDTFParser &parser)
{
    auto gdtfRoot = parser.GetGeometryRoot();
    if (!gdtfRoot)
    {
        return nullptr;
    }

    std::map<std::string, std::shared_ptr<Mesh>> meshCache;
    auto root = CreateNodeRecursive(device, parser, gdtfRoot, meshCache);

    return root;
}

std::shared_ptr<SceneGraph::Node>
GDTFLoader::CreateNodeRecursive(ID3D11Device *device, GDTFParser &parser, const std::shared_ptr<GeometryNode> &gdtfNode,
                                std::map<std::string, std::shared_ptr<Mesh>> &meshCache)
{
    if (!gdtfNode)
    {
        return nullptr;
    }

    std::shared_ptr<SceneGraph::Node> sceneNode;

    // Check if this node has a model
    if (!gdtfNode->model.empty())
    {
        std::string modelPath = parser.GetModelFile(gdtfNode->model);
        std::shared_ptr<Mesh> mesh;

        if (meshCache.count(modelPath))
        {
            mesh = meshCache[modelPath];
        }
        else
        {
            // Try to load model from GDTF
            std::vector<uint8_t> modelData;

            // Only try to load if it's a known 3D format we support (GLB/GLTF/3DS) or no extension
            bool isSupported =
                (modelPath.find(".glb") != std::string::npos || modelPath.find(".gltf") != std::string::npos ||
                 modelPath.find(".3ds") != std::string::npos || modelPath.find('.') == std::string::npos);

            if (isSupported)
            {
                // Try various search patterns
                std::vector<std::string> searchPaths;
                std::string baseName = modelPath;

                // If no extension, try .glb and .3ds
                if (baseName.find('.') == std::string::npos)
                {
                    searchPaths.push_back(baseName + ".glb");
                    searchPaths.push_back("models/" + baseName + ".glb");
                    searchPaths.push_back(baseName + ".3ds");
                    searchPaths.push_back("models/" + baseName + ".3ds");
                    searchPaths.push_back("models/3ds/" + baseName + ".3ds");
                }
                else
                {
                    searchPaths.push_back(baseName);
                    searchPaths.push_back("models/" + baseName);
                    if (baseName.find(".3ds") != std::string::npos)
                    {
                        searchPaths.push_back("models/3ds/" + baseName);
                    }
                }

                for (const auto &path : searchPaths)
                {
                    if (parser.ExtractFile(path, modelData))
                    {
                        modelPath = path; // Update hint for Assimp
                        break;
                    }
                }
            }

            if (!modelData.empty())
            {
                mesh = ModelLoader::LoadFromMemory(device, modelData.data(), modelData.size(), modelPath);
            }

            if (mesh)
            {
                meshCache[modelPath] = mesh;
                std::ofstream log("debug.log", std::ios::app);
                log << "Loaded model mesh: " << modelPath << " with Assimp.\n";
            }
        }

        if (mesh)
        {
            sceneNode = std::make_shared<SceneGraph::MeshNode>(mesh, gdtfNode->name);
        }
    }

    if (!sceneNode)
    {
        sceneNode = std::make_shared<SceneGraph::Node>(gdtfNode->name);
    }

    // Set local matrix from GDTF
    sceneNode->SetLocalMatrix(DirectX::XMLoadFloat4x4(&gdtfNode->matrix));

    for (auto &childGdtf : gdtfNode->children)
    {
        auto childScene = CreateNodeRecursive(device, parser, childGdtf, meshCache);
        if (childScene)
        {
            sceneNode->AddChild(childScene);
        }
    }

    return sceneNode;
}

} // namespace GDTF