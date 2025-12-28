#include "ScenePass.h"
#include "../../Resources/Mesh.h"
#include "../RenderTarget.h"

bool ScenePass::Initialize(ID3D11Device *device)
{
    // Load basic shader
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    if (!m_basicShader.LoadVertexShader(device, L"shaders/basic.hlsl", "VS", layout))
        return false;
    if (!m_basicShader.LoadPixelShader(device, L"shaders/basic.hlsl", "PS"))
        return false;

    // Initialize material buffer
    if (!m_materialBuffer.Initialize(device))
        return false;

    // Initialize spotlight array buffer
    if (!m_spotlightArrayBuffer.Initialize(device))
        return false;

    // Create no-cull rasterizer state for room rendering
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    HRESULT hr = device->CreateRasterizerState(&rd, &m_noCullState);
    return SUCCEEDED(hr);
}

void ScenePass::Shutdown()
{
    m_noCullState.Reset();
}

void ScenePass::Execute(ID3D11DeviceContext *context, const std::vector<Spotlight> &spotlights,
                        ID3D11DepthStencilView *dsv, ID3D11Buffer *room_vb, ID3D11Buffer *room_ib, Mesh *stage_mesh,
                        float stage_offset, float room_specular, float room_shininess)
{
    // Update spotlight buffer
    SpotlightArrayBuffer spot_data;
    std::memset(&spot_data, 0, sizeof(spot_data));

    size_t count = (std::min)(spotlights.size(), static_cast<size_t>(Config::Spotlight::MAX_SPOTLIGHTS));
    for (size_t i = 0; i < count; ++i)
    {
        spot_data.lights[i] = spotlights[i].GetGPUData();
    }
    m_spotlightArrayBuffer.Update(context, spot_data);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(Config::Display::WINDOW_WIDTH);
    viewport.Height = static_cast<float>(Config::Display::WINDOW_HEIGHT);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // Bind shader
    m_basicShader.Bind(context);

    // Bind spotlight buffer to slot 1 (matching b1 in shader)
    context->PSSetConstantBuffers(1, 1, m_spotlightArrayBuffer.GetAddressOf());

    // Render Room with dark gray material
    {
        MaterialBuffer mb = {};
        mb.color = {Config::Materials::ROOM_COLOR, Config::Materials::ROOM_COLOR, Config::Materials::ROOM_COLOR, 1.0f};
        mb.specParams = {room_specular, room_shininess, 0.0f, 0.0f};
        m_materialBuffer.Update(context, mb);
        context->PSSetConstantBuffers(2, 1, m_materialBuffer.GetAddressOf());

        // Use no-cull state for room (we're inside the cube)
        context->RSSetState(m_noCullState.Get());

        UINT stride = Config::Vertex::STRIDE_FULL;
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &room_vb, &stride, &offset);
        context->IASetIndexBuffer(room_ib, DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->DrawIndexed(Config::Room::INDEX_COUNT, 0, 0);

        context->RSSetState(nullptr);
    }

    // Render Stage Mesh with per-shape materials from MTL
    if (stage_mesh)
    {
        const auto &shapes = stage_mesh->GetShapes();
        for (size_t i = 0; i < shapes.size(); ++i)
        {
            const auto &shape = shapes[i];

            MaterialBuffer mb_mat = {};
            mb_mat.color = {shape.material.diffuse.x, shape.material.diffuse.y, shape.material.diffuse.z, 1.0f};
            float spec_intensity =
                (shape.material.specular.x + shape.material.specular.y + shape.material.specular.z) / 3.0f;
            mb_mat.specParams = {spec_intensity, shape.material.shininess, 0.0f, 0.0f};
            m_materialBuffer.Update(context, mb_mat);
            context->PSSetConstantBuffers(2, 1, m_materialBuffer.GetAddressOf());

            stage_mesh->DrawShape(context, i);
        }
    }
}
