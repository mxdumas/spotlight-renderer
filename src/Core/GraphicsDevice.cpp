#include "GraphicsDevice.h"
#include <iostream>

GraphicsDevice::~GraphicsDevice()
{
    Shutdown();
}

bool GraphicsDevice::Initialize(HWND hwnd)
{
    // Create swap chain description
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = Config::Display::WINDOW_WIDTH;
    sd.BufferDesc.Height = Config::Display::WINDOW_HEIGHT;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = Config::Display::REFRESH_RATE;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    UINT create_device_flags = 0;
#ifdef _DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL feature_level;

    HRESULT hr =
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_levels,
                                      1, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &feature_level, &m_context);

    if (FAILED(hr))
    {
        return false;
    }

    // Get back buffer and create RTV
    ComPtr<ID3D11Texture2D> back_buffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_backBufferRTV);
    if (FAILED(hr))
        return false;

    // Create depth/stencil buffer
    D3D11_TEXTURE2D_DESC depth_desc = {};
    depth_desc.Width = Config::Display::WINDOW_WIDTH;
    depth_desc.Height = Config::Display::WINDOW_HEIGHT;
    depth_desc.MipLevels = 1;
    depth_desc.ArraySize = 1;
    depth_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depth_desc.SampleDesc.Count = 1;
    depth_desc.SampleDesc.Quality = 0;
    depth_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    hr = m_device->CreateTexture2D(&depth_desc, nullptr, &m_depthStencilBuffer);
    if (FAILED(hr))
        return false;

    // Create DSV
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsv_desc, &m_depthStencilView);
    if (FAILED(hr))
        return false;

    // Create depth SRV for reading depth in shaders
    D3D11_SHADER_RESOURCE_VIEW_DESC depth_srv_desc = {};
    depth_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depth_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depth_srv_desc.Texture2D.MipLevels = 1;
    hr = m_device->CreateShaderResourceView(m_depthStencilBuffer.Get(), &depth_srv_desc, &m_depthSRV);
    if (FAILED(hr))
        return false;

    // Set initial render target and viewport
    SetBackBufferAsRenderTarget();
    SetDefaultViewport();

    return true;
}

void GraphicsDevice::Shutdown()
{
    m_depthSRV.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();
    m_backBufferRTV.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}

void GraphicsDevice::Present(bool vsync)
{
    m_swapChain->Present(vsync ? 1 : 0, 0);
}

void GraphicsDevice::ClearBackBuffer(const float *color)
{
    m_context->ClearRenderTargetView(m_backBufferRTV.Get(), color);
}

void GraphicsDevice::ClearDepthStencil()
{
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void GraphicsDevice::SetBackBufferAsRenderTarget()
{
    m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_depthStencilView.Get());
}

void GraphicsDevice::SetViewport(float width, float height)
{
    D3D11_VIEWPORT viewport = {};
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    m_context->RSSetViewports(1, &viewport);
}

void GraphicsDevice::SetDefaultViewport()
{
    SetViewport(static_cast<float>(Config::Display::WINDOW_WIDTH), static_cast<float>(Config::Display::WINDOW_HEIGHT));
}
