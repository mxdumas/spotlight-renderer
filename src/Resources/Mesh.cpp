#define TINYOBJLOADER_IMPLEMENTATION
#include "Mesh.h"
#include "tiny_obj_loader.h"

Mesh::Mesh() = default;

bool Mesh::LoadFromOBJ(ID3D11Device *device, const std::string &fileName)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    [[maybe_unused]] std::string warn;
    [[maybe_unused]] std::string err;

    // Extract directory path for MTL file loading
    std::string mtlBaseDir;
    size_t lastSlash = fileName.find_last_of("/\\\\");
    if (lastSlash != std::string::npos)
    {
        mtlBaseDir = fileName.substr(0, lastSlash + 1);
    }

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str(), mtlBaseDir.c_str()))
    {
        return false;
    }

    // Build material data from MTL
    std::vector<MaterialData> materialDataList;
    for (const auto &mat : materials)
    {
        MaterialData md;
        md.diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]};
        md.specular = {mat.specular[0], mat.specular[1], mat.specular[2]};
        md.shininess = mat.shininess > 0.0f ? mat.shininess : 32.0f;
        materialDataList.push_back(md);
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float globalMinY = FLT_MAX;
    uint32_t currentIndex = 0;

    for (const auto &shape : shapes)
    {
        ShapeInfo info;
        info.name = shape.name;
        info.startIndex = currentIndex;

        float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

        for (const auto &index : shape.mesh.indices)
        {
            Vertex vertex = {};

            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];

            minX = (std::min)(minX, vx);
            minY = (std::min)(minY, vy);
            minZ = (std::min)(minZ, vz);
            maxX = (std::max)(maxX, vx);
            maxY = (std::max)(maxY, vy);
            maxZ = (std::max)(maxZ, vz);

            globalMinY = (std::min)(globalMinY, vy);

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
        currentIndex += info.indexCount;

        // Get material from first face (shapes typically use one material)
        if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0)
        {
            size_t matId = (size_t)shape.mesh.material_ids[0];
            if (matId < materialDataList.size())
            {
                info.material = materialDataList[matId];
            }
        }

        info.center = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f};
        m_shapes.push_back(info);
    }

    m_minY = globalMinY;
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

    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vbd, &vinitData, &m_vertexBuffer);
    if (FAILED(hr))
        return false;

    // Create index buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(uint32_t) * m_indexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices.data();

    hr = device->CreateBuffer(&ibd, &iinitData, &m_indexBuffer);
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

void Mesh::DrawShape(ID3D11DeviceContext *context, size_t shapeIndex)
{
    if (shapeIndex >= m_shapes.size())
        return;
    const auto &shape = m_shapes[shapeIndex];

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(shape.indexCount, shape.startIndex, 0);
}