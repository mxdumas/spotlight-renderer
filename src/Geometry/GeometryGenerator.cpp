#include "GeometryGenerator.h"
#include <cmath>
#include <vector>
#include "../Core/Config.h"

namespace GeometryGenerator
{

bool CreateDebugCube(ID3D11Device *device, ComPtr<ID3D11Buffer> &outVb, ComPtr<ID3D11Buffer> &outIb)
{
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,  0.5f, -0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    };
    uint32_t indices[] = {0, 2, 1, 0, 3, 2, 1, 6, 5, 1, 2, 6, 5, 7, 4, 5, 6, 7,
                          4, 3, 0, 4, 7, 3, 3, 6, 2, 3, 7, 6, 4, 1, 5, 4, 0, 1};

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {vertices};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &outVb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {indices};
    hr = device->CreateBuffer(&ibd, &iinit, &outIb);
    return SUCCEEDED(hr);
}

bool CreateConeProxy(ID3D11Device *device, ComPtr<ID3D11Buffer> &outVb, ComPtr<ID3D11Buffer> &outIb,
                     uint32_t &outIndexCount)
{
    std::vector<float> coneVertices;
    std::vector<uint32_t> coneIndices;

    // Tip vertex at origin
    coneVertices.push_back(0.0f);
    coneVertices.push_back(0.0f);
    coneVertices.push_back(0.0f);

    constexpr int segments = Config::Geometry::CONE_SEGMENTS;
    constexpr float radius = Config::Geometry::CONE_RADIUS;
    constexpr float height = Config::Geometry::CONE_HEIGHT;

    // Base circle vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) / segments * 2.0f * Config::Math::PI;
        coneVertices.push_back(std::cos(angle) * radius);
        coneVertices.push_back(std::sin(angle) * radius);
        coneVertices.push_back(height);
    }

    // Indices: lines from tip + base circle lines
    for (int i = 0; i < segments; ++i)
    {
        // Lines from tip to base
        coneIndices.push_back(0);
        coneIndices.push_back(i + 1);

        // Lines for base circle
        coneIndices.push_back(i + 1);
        coneIndices.push_back(((i + 1) % segments) + 1);
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(coneVertices.size() * sizeof(float));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {coneVertices.data()};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &outVb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(coneIndices.size() * sizeof(uint32_t));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {coneIndices.data()};
    hr = device->CreateBuffer(&ibd, &iinit, &outIb);
    if (FAILED(hr))
        return false;

    outIndexCount = static_cast<uint32_t>(coneIndices.size());
    return true;
}

bool CreateRoomCube(ID3D11Device *device, ComPtr<ID3D11Buffer> &outVb, ComPtr<ID3D11Buffer> &outIb)
{
    constexpr float r = Config::Room::HALF_WIDTH;
    constexpr float floorY = Config::Room::FLOOR_Y;
    constexpr float ceilY = Config::Room::CEILING_Y;

    // Vertex format: position (3) + normal (3) + uv (2) = 8 floats per vertex
    // Normals inverted to face inward
    float roomVerts[] = {
        // Back wall (-Z) - normal facing +Z
        -r,
        floorY,
        -r,
        0,
        0,
        1,
        0,
        1,
        r,
        floorY,
        -r,
        0,
        0,
        1,
        1,
        1,
        r,
        ceilY,
        -r,
        0,
        0,
        1,
        1,
        0,
        -r,
        ceilY,
        -r,
        0,
        0,
        1,
        0,
        0,

        // Front wall (+Z) - normal facing -Z
        -r,
        floorY,
        r,
        0,
        0,
        -1,
        0,
        1,
        r,
        floorY,
        r,
        0,
        0,
        -1,
        1,
        1,
        r,
        ceilY,
        r,
        0,
        0,
        -1,
        1,
        0,
        -r,
        ceilY,
        r,
        0,
        0,
        -1,
        0,
        0,

        // Left wall (-X) - normal facing +X
        -r,
        floorY,
        r,
        1,
        0,
        0,
        0,
        1,
        -r,
        floorY,
        -r,
        1,
        0,
        0,
        1,
        1,
        -r,
        ceilY,
        -r,
        1,
        0,
        0,
        1,
        0,
        -r,
        ceilY,
        r,
        1,
        0,
        0,
        0,
        0,

        // Right wall (+X) - normal facing -X
        r,
        floorY,
        r,
        -1,
        0,
        0,
        0,
        1,
        r,
        floorY,
        -r,
        -1,
        0,
        0,
        1,
        1,
        r,
        ceilY,
        -r,
        -1,
        0,
        0,
        1,
        0,
        r,
        ceilY,
        r,
        -1,
        0,
        0,
        0,
        0,

        // Floor (-Y) - normal facing +Y
        -r,
        floorY,
        r,
        0,
        1,
        0,
        0,
        1,
        -r,
        floorY,
        -r,
        0,
        1,
        0,
        1,
        1,
        r,
        floorY,
        -r,
        0,
        1,
        0,
        1,
        0,
        r,
        floorY,
        r,
        0,
        1,
        0,
        0,
        0,

        // Ceiling (+Y) - normal facing -Y
        -r,
        ceilY,
        r,
        0,
        -1,
        0,
        0,
        1,
        -r,
        ceilY,
        -r,
        0,
        -1,
        0,
        1,
        1,
        r,
        ceilY,
        -r,
        0,
        -1,
        0,
        1,
        0,
        r,
        ceilY,
        r,
        0,
        -1,
        0,
        0,
        0,
    };

    // CCW indices for inward-facing triangles
    uint32_t roomInds[] = {// Floor
                           16, 17, 18, 16, 18, 19,
                           // Ceiling
                           20, 22, 21, 20, 23, 22,
                           // Back
                           0, 1, 2, 0, 2, 3,
                           // Front
                           4, 6, 5, 4, 7, 6,
                           // Left
                           8, 9, 10, 8, 10, 11,
                           // Right
                           12, 14, 13, 12, 15, 14};

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(roomVerts);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {roomVerts};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &outVb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(roomInds);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {roomInds};
    hr = device->CreateBuffer(&ibd, &iinit, &outIb);
    return SUCCEEDED(hr);
}

bool CreateSphere(ID3D11Device *device, ComPtr<ID3D11Buffer> &outVb, ComPtr<ID3D11Buffer> &outIb,
                  uint32_t &outIndexCount)
{
    std::vector<float> verts;
    std::vector<uint32_t> inds;

    constexpr int stacks = Config::Geometry::SPHERE_STACKS;
    constexpr int slices = Config::Geometry::SPHERE_SLICES;
    constexpr float radius = Config::Geometry::SPHERE_RADIUS;

    // Generate vertices: position (3) + normal (3) + uv (2)
    for (int i = 0; i <= stacks; ++i)
    {
        float lat = static_cast<float>(i) / stacks * Config::Math::PI;
        float y = std::cos(lat) * radius;
        float r = std::sin(lat) * radius;

        for (int j = 0; j <= slices; ++j)
        {
            float lon = static_cast<float>(j) / slices * 2.0f * Config::Math::PI;
            float x = std::cos(lon) * r;
            float z = std::sin(lon) * r;

            // Position
            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);
            // Normal (normalized position)
            verts.push_back(x / radius);
            verts.push_back(y / radius);
            verts.push_back(z / radius);
            // UV
            verts.push_back(static_cast<float>(j) / slices);
            verts.push_back(static_cast<float>(i) / stacks);
        }
    }

    // Generate indices
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;

            inds.push_back(first);
            inds.push_back(second);
            inds.push_back(first + 1);

            inds.push_back(second);
            inds.push_back(second + 1);
            inds.push_back(first + 1);
        }
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(verts.size() * sizeof(float));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {verts.data()};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &outVb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(inds.size() * sizeof(uint32_t));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {inds.data()};
    hr = device->CreateBuffer(&ibd, &iinit, &outIb);
    if (FAILED(hr))
        return false;

    outIndexCount = static_cast<uint32_t>(inds.size());
    return true;
}

bool CreateFullScreenQuad(ID3D11Device *device, ComPtr<ID3D11Buffer> &outVb)
{
    // Two triangles covering NDC [-1,1] x [-1,1]
    // Position only (3 floats per vertex, 6 vertices)
    float fsVertices[] = {-1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
                          1.0f,  -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  0.0f};

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(fsVertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {fsVertices};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &outVb);
    return SUCCEEDED(hr);
}

} // namespace GeometryGenerator