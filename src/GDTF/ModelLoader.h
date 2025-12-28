/**
 * @file ModelLoader.h
 * @brief Assimp-based 3D model loading for GDTF fixtures.
 */

#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include "../Resources/Mesh.h"

namespace GDTF
{

/**
 * @class ModelLoader
 * @brief Unified model loader using Assimp to support 3DS, GLB, OBJ, etc.
 */
class ModelLoader
{
public:
    /**
     * @brief Loads a mesh from binary data in memory.
     *
     * @param device Pointer to D3D11 device.
     * @param data Pointer to raw binary data.
     * @param size Size of data in bytes.
     * @param hint Extension hint (e.g., ".3ds", ".glb").
     * @return Shared pointer to Mesh, or nullptr on failure.
     */
    static std::shared_ptr<Mesh> LoadFromMemory(ID3D11Device *device, const uint8_t *data, size_t size,
                                                const std::string &hint);
};

} // namespace GDTF
