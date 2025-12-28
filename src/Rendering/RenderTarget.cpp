#include "RenderTarget.h"

RenderTarget::~RenderTarget()
{
    Shutdown();
}

bool RenderTarget::Create(ID3D11Device *device, int width, int height, DXGI_FORMAT format)
{
    if (!device || width <= 0 || height <= 0)
    {
        return false;
    }

    // Release any existing resources
    Shutdown();

    m_width = width;
    m_height = height;

    // Create texture
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = static_cast<UINT>(width);
    tex_desc.Height = static_cast<UINT>(height);
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&tex_desc, nullptr, &m_texture);
    if (FAILED(hr))
    {
        return false;
    }

    // Create render target view
    hr = device->CreateRenderTargetView(m_texture.Get(), nullptr, &m_rtv);
    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    // Create shader resource view
    hr = device->CreateShaderResourceView(m_texture.Get(), nullptr, &m_srv);
    if (FAILED(hr))
    {
        Shutdown();
        return false;
    }

    return true;
}

void RenderTarget::Shutdown()
{
    m_srv.Reset();
    m_rtv.Reset();
    m_texture.Reset();
    m_width = 0;
    m_height = 0;
}

void RenderTarget::Bind(ID3D11DeviceContext *context, ID3D11DepthStencilView *dsv)
{
    if (context && m_rtv)
    {
        context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), dsv);
    }
}

void RenderTarget::Clear(ID3D11DeviceContext *context, const float *color)
{
    if (context && m_rtv && color)
    {
        context->ClearRenderTargetView(m_rtv.Get(), color);
    }
}
