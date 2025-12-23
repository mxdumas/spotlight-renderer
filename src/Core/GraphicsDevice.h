#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Config.h"

using Microsoft::WRL::ComPtr;

class GraphicsDevice {
public:
    GraphicsDevice() = default;
    ~GraphicsDevice();

    // Initialization and shutdown
    bool Initialize(HWND hwnd);
    void Shutdown();

    // Frame operations
    void Present(bool vsync = true);
    void ClearBackBuffer(const float* color);
    void ClearDepthStencil();

    // Accessors
    ID3D11Device* GetDevice() const { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_context.Get(); }
    IDXGISwapChain* GetSwapChain() const { return m_swapChain.Get(); }

    ID3D11RenderTargetView* GetBackBufferRTV() const { return m_backBufferRTV.Get(); }
    ID3D11DepthStencilView* GetDepthStencilView() const { return m_depthStencilView.Get(); }
    ID3D11ShaderResourceView* GetDepthSRV() const { return m_depthSRV.Get(); }

    // Utility methods
    void SetBackBufferAsRenderTarget();
    void SetViewport(float width, float height);
    void SetDefaultViewport();

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_backBufferRTV;
    ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11ShaderResourceView> m_depthSRV;
};
