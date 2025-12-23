#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
};

struct ShapeInfo {
    std::string name;
    DirectX::XMFLOAT3 center;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool LoadFromOBJ(ID3D11Device* device, const std::string& fileName);
    void Draw(ID3D11DeviceContext* context);

    const std::vector<ShapeInfo>& GetShapes() const { return m_shapes; }

private:
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    UINT m_indexCount;

    std::vector<ShapeInfo> m_shapes;
};
