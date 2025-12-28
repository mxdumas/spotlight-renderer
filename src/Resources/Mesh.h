#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <string>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * @struct Vertex
 * @brief Represents a single vertex in a 3D mesh.
 */
struct Vertex
{
    DirectX::XMFLOAT3 position; ///< 3D position of the vertex.
    DirectX::XMFLOAT3 normal;   ///< Normal vector for lighting calculations.
    DirectX::XMFLOAT2 uv;       ///< Texture coordinates.
};

/**
 * @struct MaterialData
 * @brief Material properties loaded from MTL files.
 */
struct MaterialData
{
    DirectX::XMFLOAT3 diffuse = {1.0f, 1.0f, 1.0f};  ///< Diffuse color (Kd).
    DirectX::XMFLOAT3 specular = {0.5f, 0.5f, 0.5f}; ///< Specular color (Ks).
    float shininess = 32.0f;                         ///< Shininess exponent (Ns).
};

/**
 * @struct ShapeInfo
 * @brief Metadata about a specific shape or object within a mesh file.
 */
struct ShapeInfo
{
    std::string name;         ///< Name of the shape.
    DirectX::XMFLOAT3 center; ///< Computed center point of the shape.
    MaterialData material;    ///< Material properties for this shape.
    uint32_t startIndex = 0;  ///< Starting index in the index buffer.
    uint32_t indexCount = 0;  ///< Number of indices for this shape.
};

/**
 * @class Mesh
 * @brief Represents a 3D geometry loaded from an external file.
 *
 * This class handles the loading of OBJ files, creates GPU buffers (vertex and index),
 * and provides a method to draw the geometry.
 */
class Mesh
{
public:
    /**
     * @brief Default constructor for the Mesh class.
     */
    Mesh();

    /**
     * @brief Destructor for the Mesh class.
     * Releases GPU resources.
     */
    ~Mesh() = default;

    /**
     * @brief Loads a 3D model from an OBJ file and creates DirectX 11 buffers.
     *
     * @param device Pointer to the ID3D11Device.
     * @param fileName Path to the .obj file.
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadFromOBJ(ID3D11Device *device, const std::string &fileName);

    /**
     * @brief Creates a mesh from raw vertex and index data.
     *
     * @param device Pointer to the ID3D11Device.
     * @param vertices Vector of vertices.
     * @param indices Vector of indices.
     * @return true if successful.
     */
    bool Create(ID3D11Device *device, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

    /**
     * @brief Adds a shape to the mesh.
     * @param info Shape metadata.
     */
    void AddShape(const ShapeInfo &info)
    {
        m_shapes.push_back(info);
    }

    /**
     * @brief Binds the vertex and index buffers and issues a draw call for entire mesh.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     */
    void Draw(ID3D11DeviceContext *context);

    /**
     * @brief Draws a single shape from the mesh by index.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param shapeIndex Index of the shape to draw.
     */
    void DrawShape(ID3D11DeviceContext *context, size_t shapeIndex);

    /**
     * @brief Gets the metadata for all shapes found in the mesh file.
     * @return Const reference to a vector of ShapeInfo.
     */
    [[nodiscard]] const std::vector<ShapeInfo> &GetShapes() const
    {
        return m_shapes;
    }

    /**
     * @brief Gets the minimum Y coordinate found in the mesh (useful for floor placement).
     * @return The minimum Y value.
     */
    [[nodiscard]] float GetMinY() const
    {
        return m_minY;
    }

private:
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    UINT m_indexCount{0};

    std::vector<ShapeInfo> m_shapes;
    float m_minY{0.0f};
};
