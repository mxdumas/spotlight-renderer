#pragma once

#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * @namespace GeometryGenerator
 * @brief Provides utility functions for procedurally generating simple 3D geometry.
 */
#include <vector>
#include "../Resources/Mesh.h"

// ...
namespace GeometryGenerator
{

/**
 * @brief Generates raw vertex and index data for a box.
 */
void CreateBox(float width, float height, float depth, std::vector<Vertex> &out_vertices,
               std::vector<uint32_t> &out_indices);

/**
 * @brief Creates a simple debug cube (8 vertices).
// ...
 *
 * @param device Pointer to the ID3D11Device.
 * @param out_vb Reference to the ComPtr that will receive the vertex buffer (position only).
 * @param out_ib Reference to the ComPtr that will receive the index buffer.
 * @return true if creation succeeded, false otherwise.
 */
bool CreateDebugCube(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib);

/**
 * @brief Creates a cone proxy used for visualizing spotlight orientation.
 *
 * @param device Pointer to the ID3D11Device.
 * @param out_vb Reference to the ComPtr that will receive the vertex buffer (position only).
 * @param out_ib Reference to the ComPtr that will receive the index buffer.
 * @param out_index_count Receives the number of indices in the generated cone.
 * @return true if creation succeeded, false otherwise.
 */
bool CreateConeProxy(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib,
                     uint32_t &out_index_count);

/**
 * @brief Creates an inverted room cube where normals face inward.
 *
 * @param device Pointer to the ID3D11Device.
 * @param out_vb Reference to the ComPtr that will receive the vertex buffer (position, normal, uv).
 * @param out_ib Reference to the ComPtr that will receive the index buffer.
 * @return true if creation succeeded, false otherwise.
 */
bool CreateRoomCube(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib);

/**
 * @brief Creates a UV sphere for point light visualization.
 *
 * @param device Pointer to the ID3D11Device.
 * @param out_vb Reference to the ComPtr that will receive the vertex buffer (position, normal, uv).
 * @param out_ib Reference to the ComPtr that will receive the index buffer.
 * @param out_index_count Receives the number of indices in the generated sphere.
 * @return true if creation succeeded, false otherwise.
 */
bool CreateSphere(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib,
                  uint32_t &out_index_count);

/**
 * @brief Creates a full-screen quad (2 triangles) covering NDC space from [-1,-1] to [1,1].
 *
 * @param device Pointer to the ID3D11Device.
 * @param out_vb Reference to the ComPtr that will receive the vertex buffer (position only).
 * @return true if creation succeeded, false otherwise.
 */
bool CreateFullScreenQuad(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb);

} // namespace GeometryGenerator
