#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

namespace GeometryGenerator {

    // Debug cube - simple 8-vertex box for light visualization
    // Returns: VB (position only, stride=12), IB, 36 indices
    bool CreateDebugCube(ID3D11Device* device,
                         ComPtr<ID3D11Buffer>& outVB,
                         ComPtr<ID3D11Buffer>& outIB);

    // Spotlight cone proxy - wireframe cone for visualizing light direction
    // Returns: VB (position only, stride=12), IB, outIndexCount indices
    bool CreateConeProxy(ID3D11Device* device,
                         ComPtr<ID3D11Buffer>& outVB,
                         ComPtr<ID3D11Buffer>& outIB,
                         uint32_t& outIndexCount);

    // Room cube - inverted box with normals facing inward
    // Returns: VB (position+normal+uv, stride=32), IB, 36 indices
    bool CreateRoomCube(ID3D11Device* device,
                        ComPtr<ID3D11Buffer>& outVB,
                        ComPtr<ID3D11Buffer>& outIB);

    // Debug sphere - UV sphere for point light visualization
    // Returns: VB (position+normal+uv, stride=32), IB, outIndexCount indices
    bool CreateSphere(ID3D11Device* device,
                      ComPtr<ID3D11Buffer>& outVB,
                      ComPtr<ID3D11Buffer>& outIB,
                      uint32_t& outIndexCount);

    // Full screen quad - 2 triangles covering NDC [-1,1]
    // Returns: VB (position only, stride=12), 6 vertices (no IB needed)
    bool CreateFullScreenQuad(ID3D11Device* device,
                              ComPtr<ID3D11Buffer>& outVB);

} // namespace GeometryGenerator
