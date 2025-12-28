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

    float minX = 1e10f, minY = 1e10f, minZ = 1e10f;
    float maxX = -1e10f, maxY = -1e10f, maxZ = -1e10f;

    auto mesh = std::make_shared<Mesh>();
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;
    uint32_t vertexOffset = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        aiMesh *aiMesh = scene->mMeshes[m];

        ShapeInfo shape;
        shape.name = aiMesh->mName.C_Str();
        shape.startIndex = (uint32_t)allIndices.size();

        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
        {
            Vertex v;
            // Scale down by 0.001 to convert from mm to m
            v.position = {aiMesh->mVertices[i].x * 0.001f, aiMesh->mVertices[i].y * 0.001f,
                          aiMesh->mVertices[i].z * 0.001f};

            minX = (std::min)(minX, v.position.x);
            minY = (std::min)(minY, v.position.y);
            minZ = (std::min)(minZ, v.position.z);
            maxX = (std::max)(maxX, v.position.x);
            maxY = (std::max)(maxY, v.position.y);
            maxZ = (std::max)(maxZ, v.position.z);

            if (aiMesh->HasNormals())
                v.normal = {aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z};
            else
                v.normal = {0, 1, 0};

            if (aiMesh->HasTextureCoords(0))
                v.uv = {aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y};
            else
                v.uv = {0, 0};

            allVertices.push_back(v);
        }

        for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i)
        {
            aiFace face = aiMesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                allIndices.push_back(face.mIndices[j] + vertexOffset);
        }

        shape.indexCount = aiMesh->mNumFaces * 3;
        shape.center = {0, 0, 0};
        // Assign a black material
        shape.material.diffuse = {0.05f, 0.05f, 0.05f};
        shape.material.specular = {0.2f, 0.2f, 0.2f};
        shape.material.shininess = 32.0f;

        mesh->AddShape(shape);
        vertexOffset += aiMesh->mNumVertices;
    }

    if (!mesh->Create(device, allVertices, allIndices))
        return nullptr;

    {
        std::ofstream log("debug.log", std::ios::app);
        log << "Assimp loaded " << hint << ": " << allVertices.size() << " vertices, " << allIndices.size() / 3
            << " faces.\n";
        log << "  Bounds: Min(" << minX << "," << minY << "," << minZ << ") Max(" << maxX << "," << maxY << "," << maxZ
            << ")\n";
    }

    return mesh;
}

} // namespace GDTF
