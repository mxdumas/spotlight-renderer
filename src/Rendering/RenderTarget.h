#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();

    // Non-copyable, movable
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&&) noexcept = default;
    RenderTarget& operator=(RenderTarget&&) noexcept = default;

    // Create a render target texture with optional depth stencil
    bool Create(ID3D11Device* device, int width, int height,
                DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

    // Release resources
    void Shutdown();

    // Bind as render target (with optional depth stencil from external source)
    void Bind(ID3D11DeviceContext* context, ID3D11DepthStencilView* dsv = nullptr);

    // Clear the render target
    void Clear(ID3D11DeviceContext* context, const float* color);

    // Accessors
    ID3D11Texture2D* GetTexture() const { return m_texture.Get(); }
    ID3D11RenderTargetView* GetRTV() const { return m_rtv.Get(); }
    ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

    // For use with GetAddressOf pattern in DirectX calls
    ID3D11RenderTargetView* const* GetRTVAddressOf() const { return m_rtv.GetAddressOf(); }
    ID3D11ShaderResourceView* const* GetSRVAddressOf() const { return m_srv.GetAddressOf(); }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11RenderTargetView> m_rtv;
    ComPtr<ID3D11ShaderResourceView> m_srv;
    int m_width = 0;
    int m_height = 0;
};
