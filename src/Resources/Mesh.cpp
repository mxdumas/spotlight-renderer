#define TINYOBJLOADER_IMPLEMENTATION
#include "Mesh.h"
#include "tiny_obj_loader.h"

Mesh::Mesh() = default;

bool Mesh::LoadFromOBJ(ID3D11Device *device, const std::string &file_name)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Extract directory path for MTL file loading
    std::string mtl_base_dir;
    size_t last_slash = file_name.find_last_of("/\\\\");
    if (last_slash != std::string::npos)
    {
        mtl_base_dir = file_name.substr(0, last_slash + 1);
    }

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name.c_str(), mtl_base_dir.c_str()))
    {
        return false;
    }

    // Build material data from MTL
    std::vector<MaterialData> material_data_list;
    for (const auto &mat : materials)
    {
        MaterialData md;
        md.diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]};
        md.specular = {mat.specular[0], mat.specular[1], mat.specular[2]};
        md.shininess = mat.shininess > 0.0f ? mat.shininess : 32.0f;
        material_data_list.push_back(md);
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float global_min_y = FLT_MAX;
    uint32_t current_index = 0;

    for (const auto &shape : shapes)
    {
        ShapeInfo info;
        info.name = shape.name;
        info.startIndex = current_index;

        float min_x = FLT_MAX, min_y = FLT_MAX, min_z = FLT_MAX;
        float max_x = -FLT_MAX, max_y = -FLT_MAX, max_z = -FLT_MAX;

        for (const auto &index : shape.mesh.indices)
        {
            Vertex vertex = {};

            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];

            min_x = (std::min)(min_x, vx);
            min_y = (std::min)(min_y, vy);
            min_z = (std::min)(min_z, vz);
            max_x = (std::max)(max_x, vx);
            max_y = (std::max)(max_y, vy);
            max_z = (std::max)(max_z, vz);

            global_min_y = (std::min)(global_min_y, vy);

            vertex.position = {vx, vy, vz};

            if (index.normal_index >= 0)
            {
                vertex.normal = {attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};
            }

            if (index.texcoord_index >= 0)
            {
                vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                             1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            }

            vertices.push_back(vertex);
            indices.push_back((uint32_t)indices.size());
        }

        info.indexCount = (uint32_t)shape.mesh.indices.size();
        current_index += info.indexCount;

        // Get material from first face (shapes typically use one material)
        if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0)
        {
            size_t mat_id = (size_t)shape.mesh.material_ids[0];
            if (mat_id < material_data_list.size())
            {
                info.material = material_data_list[mat_id];
            }
        }

        info.center = {(min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f, (min_z + max_z) * 0.5f};
        m_shapes.push_back(info);
    }

    m_minY = global_min_y;
    return Create(device, vertices, indices);
}

bool Mesh::Create(ID3D11Device *device, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
{
    m_indexCount = (UINT)indices.size();

    // Create vertex buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(Vertex) * (UINT)vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinit_data = {};
    vinit_data.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbd, &vinit_data, &m_vertexBuffer);
    if (FAILED(hr))
        return false;

    // Create index buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(uint32_t) * m_indexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit_data = {};
    iinit_data.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iinit_data, &m_indexBuffer);
    return SUCCEEDED(hr);
}

void Mesh::Draw(ID3D11DeviceContext *context)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_indexCount, 0, 0);
}

void Mesh::DrawShape(ID3D11DeviceContext *context, size_t shape_index)
{
    if (shape_index >= m_shapes.size())
        return;
    const auto &shape = m_shapes[shape_index];

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(shape.indexCount, shape.startIndex, 0);
}