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
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = static_cast<UINT>(width);
    texDesc.Height = static_cast<UINT>(height);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &m_texture);
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
