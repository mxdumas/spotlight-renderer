#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

template<typename T>
class ConstantBuffer {
public:
    ConstantBuffer() {}

    bool Initialize(ID3D11Device* device) {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = (sizeof(T) + 15) & ~15; // 16-byte alignment
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = device->CreateBuffer(&bd, nullptr, &m_buffer);
        return SUCCEEDED(hr);
    }

    void Update(ID3D11DeviceContext* context, const T& data) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            memcpy(mappedResource.pData, &data, sizeof(T));
            context->Unmap(m_buffer.Get(), 0);
        }
    }

    ID3D11Buffer* Get() const { return m_buffer.Get(); }
    ID3D11Buffer* const* GetAddressOf() const { return m_buffer.GetAddressOf(); }

private:
    ComPtr<ID3D11Buffer> m_buffer;
};
