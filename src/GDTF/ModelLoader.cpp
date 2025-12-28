#include "ModelLoader.h"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <iostream>

namespace GDTF
{

std::shared_ptr<Mesh> ModelLoader::LoadFromMemory(ID3D11Device *device, const uint8_t *data, size_t size,
                                                  const std::string &hint)
{
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
                         aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_ConvertToLeftHanded;

    const aiScene *scene = importer.ReadFileFromMemory(data, size, flags, hint.c_str());

    if (!scene || !scene->HasMeshes())
    {
        std::ofstream log("debug.log", std::ios::app);
        log << "Assimp failed to load " << hint << ": " << importer.GetErrorString() << '\n';
        return nullptr;
    }

    float min_x = 1e10f, min_y = 1e10f, min_z = 1e10f;
    float max_x = -1e10f, max_y = -1e10f, max_z = -1e10f;

    auto mesh = std::make_shared<Mesh>();
    std::vector<Vertex> all_vertices;
    std::vector<uint32_t> all_indices;
    uint32_t vertex_offset = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        aiMesh *ai_mesh = scene->mMeshes[m];

        ShapeInfo shape;
        shape.name = ai_mesh->mName.C_Str();
        shape.startIndex = (uint32_t)all_indices.size();

        for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i)
        {
            Vertex v;
            // Scale down by 0.001 to convert from mm to m
            v.position = {ai_mesh->mVertices[i].x * 0.001f, ai_mesh->mVertices[i].y * 0.001f,
                          ai_mesh->mVertices[i].z * 0.001f};

            min_x = (std::min)(min_x, v.position.x);
            min_y = (std::min)(min_y, v.position.y);
            min_z = (std::min)(min_z, v.position.z);
            max_x = (std::max)(max_x, v.position.x);
            max_y = (std::max)(max_y, v.position.y);
            max_z = (std::max)(max_z, v.position.z);

            if (ai_mesh->HasNormals())
                v.normal = {ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z};
            else
                v.normal = {0, 1, 0};

            if (ai_mesh->HasTextureCoords(0))
                v.uv = {ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y};
            else
                v.uv = {0, 0};

            all_vertices.push_back(v);
        }

        for (unsigned int i = 0; i < ai_mesh->mNumFaces; ++i)
        {
            aiFace face = ai_mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                all_indices.push_back(face.mIndices[j] + vertex_offset);
        }

        shape.indexCount = ai_mesh->mNumFaces * 3;
        shape.center = {0, 0, 0};

        // Assign a black material
        shape.material.diffuse = {0.05f, 0.05f, 0.05f};
        shape.material.specular = {0.2f, 0.2f, 0.2f};
        shape.material.shininess = 32.0f;

        mesh->AddShape(shape);

        vertex_offset += ai_mesh->mNumVertices;
    }

    if (!mesh->Create(device, all_vertices, all_indices))
        return nullptr;

    {
        std::ofstream log("debug.log", std::ios::app);
        log << "Assimp loaded " << hint << ": " << all_vertices.size() << " vertices, " << all_indices.size() / 3
            << " faces.\n";
        log << "  Bounds: Min(" << min_x << "," << min_y << "," << min_z << ") Max(" << max_x << "," << max_y << ","
            << max_z << ")\n";
    }

    return mesh;
}

} // namespace GDTF