#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * @class ConstantBuffer
 * @brief Template class for managing DirectX 11 constant buffers.
 *
 * This class simplifies the creation, updating, and usage of constant buffers,
 * ensuring proper 16-byte alignment as required by D3D11.
 *
 * @tparam T The structure type to be stored in the constant buffer.
 */
template <typename T> class ConstantBuffer
{
public:
    /**
     * @brief Default constructor for the ConstantBuffer class.
     */
    ConstantBuffer()
    {
    }

    /**
     * @brief Initializes the constant buffer on the GPU.
     *
     * @param device Pointer to the ID3D11Device.
     * @return true if initialization was successful, false otherwise.
     */
    bool Initialize(ID3D11Device *device)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = (sizeof(T) + 15) & ~15; // 16-byte alignment
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = device->CreateBuffer(&bd, nullptr, &m_buffer);
        return SUCCEEDED(hr);
    }

    /**
     * @brief Updates the constant buffer data on the GPU.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param data The new data to upload to the buffer.
     */
    void Update(ID3D11DeviceContext *context, const T &data)
    {
        if (!m_buffer)
            return;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        {
            memcpy(mappedResource.pData, &data, sizeof(T));
            context->Unmap(m_buffer.Get(), 0);
        }
    }

    /**
     * @brief Gets the underlying ID3D11Buffer pointer.
     * @return Pointer to the D3D11 buffer.
     */
    ID3D11Buffer *Get() const
    {
        return m_buffer.Get();
    }

    /**
     * @brief Gets the address of the underlying ID3D11Buffer pointer.
     * Useful for binding calls like VSSetConstantBuffers.
     * @return Address of the buffer pointer.
     */
    ID3D11Buffer *const *GetAddressOf() const
    {
        return m_buffer.GetAddressOf();
    }

private:
    ComPtr<ID3D11Buffer> m_buffer;
};
