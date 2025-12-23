#pragma once

#include "IRenderPass.h"
#include "../../Resources/Shader.h"
#include "../../Core/ConstantBuffer.h"
#include "../../Core/Config.h"
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

class Mesh;
class RenderTarget;

// Material data for shader
__declspec(align(16)) struct MaterialBuffer {
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT4 specParams; // x: intensity, y: shininess, zw: unused
};

// Scene rendering pass - renders room and stage geometry with lighting
class ScenePass : public IRenderPass {
public:
    ScenePass() = default;
    ~ScenePass() override = default;

    bool Initialize(ID3D11Device* device) override;
    void Shutdown() override;

    // Set render target for this pass
    void SetRenderTarget(RenderTarget* rt) { m_renderTarget = rt; }

    // Execute scene rendering
    // Requires external resources to be bound before calling
    void Execute(ID3D11DeviceContext* context,
                 ID3D11DepthStencilView* dsv,
                 ID3D11Buffer* roomVB,
                 ID3D11Buffer* roomIB,
                 Mesh* stageMesh,
                 float stageOffset,
                 float roomSpecular,
                 float roomShininess);

    // Access shader for external buffer binding
    Shader& GetShader() { return m_basicShader; }

    // Access material buffer for updating
    ConstantBuffer<MaterialBuffer>& GetMaterialBuffer() { return m_materialBuffer; }

private:
    Shader m_basicShader;
    ConstantBuffer<MaterialBuffer> m_materialBuffer;
    RenderTarget* m_renderTarget = nullptr;

    // Rasterizer state for room (no culling)
    ComPtr<ID3D11RasterizerState> m_noCullState;
};
