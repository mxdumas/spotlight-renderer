#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

/**
 * @namespace GeometryGenerator
 * @brief Provides utility functions for procedurally generating simple 3D geometry.
 */
namespace GeometryGenerator {

    /**
     * @brief Creates a simple debug cube (8 vertices).
     * 
     * @param device Pointer to the ID3D11Device.
     * @param outVB Reference to the ComPtr that will receive the vertex buffer (position only).
     * @param outIB Reference to the ComPtr that will receive the index buffer.
     * @return true if creation succeeded, false otherwise.
     */
    bool CreateDebugCube(ID3D11Device* device,
                         ComPtr<ID3D11Buffer>& outVB,
                         ComPtr<ID3D11Buffer>& outIB);

    /**
     * @brief Creates a cone proxy used for visualizing spotlight orientation.
     * 
     * @param device Pointer to the ID3D11Device.
     * @param outVB Reference to the ComPtr that will receive the vertex buffer (position only).
     * @param outIB Reference to the ComPtr that will receive the index buffer.
     * @param outIndexCount Receives the number of indices in the generated cone.
     * @return true if creation succeeded, false otherwise.
     */
    bool CreateConeProxy(ID3D11Device* device,
                         ComPtr<ID3D11Buffer>& outVB,
                         ComPtr<ID3D11Buffer>& outIB,
                         uint32_t& outIndexCount);

    /**
     * @brief Creates an inverted room cube where normals face inward.
     * 
     * @param device Pointer to the ID3D11Device.
     * @param outVB Reference to the ComPtr that will receive the vertex buffer (position, normal, uv).
     * @param outIB Reference to the ComPtr that will receive the index buffer.
     * @return true if creation succeeded, false otherwise.
     */
    bool CreateRoomCube(ID3D11Device* device,
                        ComPtr<ID3D11Buffer>& outVB,
                        ComPtr<ID3D11Buffer>& outIB);

    /**
     * @brief Creates a UV sphere for point light visualization.
     * 
     * @param device Pointer to the ID3D11Device.
     * @param outVB Reference to the ComPtr that will receive the vertex buffer (position, normal, uv).
     * @param outIB Reference to the ComPtr that will receive the index buffer.
     * @param outIndexCount Receives the number of indices in the generated sphere.
     * @return true if creation succeeded, false otherwise.
     */
    bool CreateSphere(ID3D11Device* device,
                      ComPtr<ID3D11Buffer>& outVB,
                      ComPtr<ID3D11Buffer>& outIB,
                      uint32_t& outIndexCount);

    /**
     * @brief Creates a full-screen quad (2 triangles) covering NDC space from [-1,-1] to [1,1].
     * 
     * @param device Pointer to the ID3D11Device.
     * @param outVB Reference to the ComPtr that will receive the vertex buffer (position only).
     * @return true if creation succeeded, false otherwise.
     */
    bool CreateFullScreenQuad(ID3D11Device* device,
                              ComPtr<ID3D11Buffer>& outVB);

} // namespace GeometryGenerator
