#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

/**
 * @struct Vertex
 * @brief Represents a single vertex in a 3D mesh.
 */
struct Vertex {
    DirectX::XMFLOAT3 position; ///< 3D position of the vertex.
    DirectX::XMFLOAT3 normal;   ///< Normal vector for lighting calculations.
    DirectX::XMFLOAT2 uv;       ///< Texture coordinates.
};

/**
 * @struct ShapeInfo
 * @brief Metadata about a specific shape or object within a mesh file.
 */
struct ShapeInfo {
    std::string name;           ///< Name of the shape.
    DirectX::XMFLOAT3 center;    ///< Computed center point of the shape.
};

/**
 * @class Mesh
 * @brief Represents a 3D geometry loaded from an external file.
 * 
 * This class handles the loading of OBJ files, creates GPU buffers (vertex and index),
 * and provides a method to draw the geometry.
 */
class Mesh {
public:
    /**
     * @brief Default constructor for the Mesh class.
     */
    Mesh();

    /**
     * @brief Destructor for the Mesh class.
     * Releases GPU resources.
     */
    ~Mesh();

    /**
     * @brief Loads a 3D model from an OBJ file and creates DirectX 11 buffers.
     * 
     * @param device Pointer to the ID3D11Device.
     * @param fileName Path to the .obj file.
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadFromOBJ(ID3D11Device* device, const std::string& fileName);

    /**
     * @brief Binds the vertex and index buffers and issues a draw call.
     * 
     * @param context Pointer to the ID3D11DeviceContext.
     */
    void Draw(ID3D11DeviceContext* context);

    /**
     * @brief Gets the metadata for all shapes found in the mesh file.
     * @return Const reference to a vector of ShapeInfo.
     */
    const std::vector<ShapeInfo>& GetShapes() const { return m_shapes; }

    /**
     * @brief Gets the minimum Y coordinate found in the mesh (useful for floor placement).
     * @return The minimum Y value.
     */
    float GetMinY() const { return m_minY; }

private:
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    UINT m_indexCount;

    std::vector<ShapeInfo> m_shapes;
    float m_minY;
};
