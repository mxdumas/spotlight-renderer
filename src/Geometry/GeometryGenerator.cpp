#include "GeometryGenerator.h"
#include <cmath>
#include <vector>
#include "../Core/Config.h"

namespace GeometryGenerator
{

bool CreateDebugCube(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib)
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
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &out_vb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(indices);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {indices};
    hr = device->CreateBuffer(&ibd, &iinit, &out_ib);
    return SUCCEEDED(hr);
}

bool CreateConeProxy(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib,
                     uint32_t &out_index_count)
{
    std::vector<float> cone_vertices;
    std::vector<uint32_t> cone_indices;

    // Tip vertex at origin
    cone_vertices.push_back(0.0f);
    cone_vertices.push_back(0.0f);
    cone_vertices.push_back(0.0f);

    constexpr int segments = Config::Geometry::CONE_SEGMENTS;
    constexpr float radius = Config::Geometry::CONE_RADIUS;
    constexpr float height = Config::Geometry::CONE_HEIGHT;

    // Base circle vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) / segments * 2.0f * Config::Math::PI;
        cone_vertices.push_back(std::cos(angle) * radius);
        cone_vertices.push_back(std::sin(angle) * radius);
        cone_vertices.push_back(height);
    }

    // Indices: lines from tip + base circle lines
    for (int i = 0; i < segments; ++i)
    {
        // Lines from tip to base
        cone_indices.push_back(0);
        cone_indices.push_back(i + 1);

        // Lines for base circle
        cone_indices.push_back(i + 1);
        cone_indices.push_back(((i + 1) % segments) + 1);
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = static_cast<UINT>(cone_vertices.size() * sizeof(float));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {cone_vertices.data()};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &out_vb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(cone_indices.size() * sizeof(uint32_t));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {cone_indices.data()};
    hr = device->CreateBuffer(&ibd, &iinit, &out_ib);
    if (FAILED(hr))
        return false;

    out_index_count = static_cast<uint32_t>(cone_indices.size());
    return true;
}

bool CreateRoomCube(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib)
{
    constexpr float r = Config::Room::HALF_WIDTH;
    constexpr float floor_y = Config::Room::FLOOR_Y;
    constexpr float ceil_y = Config::Room::CEILING_Y;

    // Vertex format: position (3) + normal (3) + uv (2) = 8 floats per vertex
    // Normals inverted to face inward
    float room_verts[] = {
        // Back wall (-Z) - normal facing +Z
        -r,
        floor_y,
        -r,
        0,
        0,
        1,
        0,
        1,
        r,
        floor_y,
        -r,
        0,
        0,
        1,
        1,
        1,
        r,
        ceil_y,
        -r,
        0,
        0,
        1,
        1,
        0,
        -r,
        ceil_y,
        -r,
        0,
        0,
        1,
        0,
        0,
        // Front wall (+Z) - normal facing -Z
        -r,
        floor_y,
        r,
        0,
        0,
        -1,
        0,
        1,
        r,
        floor_y,
        r,
        0,
        0,
        -1,
        1,
        1,
        r,
        ceil_y,
        r,
        0,
        0,
        -1,
        1,
        0,
        -r,
        ceil_y,
        r,
        0,
        0,
        -1,
        0,
        0,
        // Left wall (-X) - normal facing +X
        -r,
        floor_y,
        r,
        1,
        0,
        0,
        0,
        1,
        -r,
        floor_y,
        -r,
        1,
        0,
        0,
        1,
        1,
        -r,
        ceil_y,
        -r,
        1,
        0,
        0,
        1,
        0,
        -r,
        ceil_y,
        r,
        1,
        0,
        0,
        0,
        0,
        // Right wall (+X) - normal facing -X
        r,
        floor_y,
        r,
        -1,
        0,
        0,
        0,
        1,
        r,
        floor_y,
        -r,
        -1,
        0,
        0,
        1,
        1,
        r,
        ceil_y,
        -r,
        -1,
        0,
        0,
        1,
        0,
        r,
        ceil_y,
        r,
        -1,
        0,
        0,
        0,
        0,
        // Floor (-Y) - normal facing +Y
        -r,
        floor_y,
        r,
        0,
        1,
        0,
        0,
        1,
        -r,
        floor_y,
        -r,
        0,
        1,
        0,
        1,
        1,
        r,
        floor_y,
        -r,
        0,
        1,
        0,
        1,
        0,
        r,
        floor_y,
        r,
        0,
        1,
        0,
        0,
        0,
        // Ceiling (+Y) - normal facing -Y
        -r,
        ceil_y,
        r,
        0,
        -1,
        0,
        0,
        1,
        -r,
        ceil_y,
        -r,
        0,
        -1,
        0,
        1,
        1,
        r,
        ceil_y,
        -r,
        0,
        -1,
        0,
        1,
        0,
        r,
        ceil_y,
        r,
        0,
        -1,
        0,
        0,
        0,
    };

    // CCW indices for inward-facing triangles
    uint32_t room_inds[] = {// Floor
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
    vbd.ByteWidth = sizeof(room_verts);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {room_verts};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &out_vb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(room_inds);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {room_inds};
    hr = device->CreateBuffer(&ibd, &iinit, &out_ib);
    return SUCCEEDED(hr);
}

bool CreateSphere(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb, ComPtr<ID3D11Buffer> &out_ib,
                  uint32_t &out_index_count)
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
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &out_vb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(inds.size() * sizeof(uint32_t));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit = {inds.data()};
    hr = device->CreateBuffer(&ibd, &iinit, &out_ib);
    if (FAILED(hr))
        return false;

    out_index_count = static_cast<uint32_t>(inds.size());
    return true;
}

bool CreateFullScreenQuad(ID3D11Device *device, ComPtr<ID3D11Buffer> &out_vb)
{
    // Two triangles covering NDC [-1,1] x [-1,1]
    // Position only (3 floats per vertex, 6 vertices)
    float fs_vertices[] = {-1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
                           1.0f,  -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  0.0f};

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(fs_vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vinit = {fs_vertices};
    HRESULT hr = device->CreateBuffer(&vbd, &vinit, &out_vb);
    return SUCCEEDED(hr);
}

void CreateBox(float width, float height, float depth, std::vector<Vertex> &out_vertices,
               std::vector<uint32_t> &out_indices)
{
    float w2 = width * 0.5f;
    float h2 = height * 0.5f;
    float d2 = depth * 0.5f;

    out_vertices.clear();
    out_indices.clear();

    struct RawVert
    {
        float p[3];
        float n[3];
        float u[2];
    };

    RawVert rv[] = {
        // Front
        {{-w2, -h2, d2}, {0, 0, 1}, {0, 1}},
        {{w2, -h2, d2}, {0, 0, 1}, {1, 1}},
        {{w2, h2, d2}, {0, 0, 1}, {1, 0}},
        {{-w2, h2, d2}, {0, 0, 1}, {0, 0}},
        // Back
        {{w2, -h2, -d2}, {0, 0, -1}, {0, 1}},
        {{-w2, -h2, -d2}, {0, 0, -1}, {1, 1}},
        {{-w2, h2, -d2}, {0, 0, -1}, {1, 0}},
        {{w2, h2, -d2}, {0, 0, -1}, {0, 0}},
        // Top
        {{-w2, h2, d2}, {0, 1, 0}, {0, 1}},
        {{w2, h2, d2}, {0, 1, 0}, {1, 1}},
        {{w2, h2, -d2}, {0, 1, 0}, {1, 0}},
        {{-w2, h2, -d2}, {0, 1, 0}, {0, 0}},
        // Bottom
        {{-w2, -h2, -d2}, {0, -1, 0}, {0, 1}},
        {{w2, -h2, -d2}, {0, -1, 0}, {1, 1}},
        {{w2, -h2, d2}, {0, -1, 0}, {1, 0}},
        {{-w2, -h2, d2}, {0, -1, 0}, {0, 0}},
        // Left
        {{-w2, -h2, -d2}, {-1, 0, 0}, {0, 1}},
        {{-w2, -h2, d2}, {-1, 0, 0}, {1, 1}},
        {{-w2, h2, d2}, {-1, 0, 0}, {1, 0}},
        {{-w2, h2, -d2}, {-1, 0, 0}, {0, 0}},
        // Right
        {{w2, -h2, d2}, {1, 0, 0}, {0, 1}},
        {{w2, -h2, -d2}, {1, 0, 0}, {1, 1}},
        {{w2, h2, -d2}, {1, 0, 0}, {1, 0}},
        {{w2, h2, d2}, {1, 0, 0}, {0, 0}},
    };

    for (const auto &raw_vertex : rv)
    {
        Vertex v;
        v.position = {raw_vertex.p[0], raw_vertex.p[1], raw_vertex.p[2]};
        v.normal = {raw_vertex.n[0], raw_vertex.n[1], raw_vertex.n[2]};
        v.uv = {raw_vertex.u[0], raw_vertex.u[1]};
        out_vertices.push_back(v);
    }

    for (uint32_t i = 0; i < 6; ++i)
    {
        uint32_t base = i * 4;
        out_indices.push_back(base);
        out_indices.push_back(base + 1);
        out_indices.push_back(base + 2);
        out_indices.push_back(base);
        out_indices.push_back(base + 2);
        out_indices.push_back(base + 3);
    }
}

} // namespace GeometryGenerator