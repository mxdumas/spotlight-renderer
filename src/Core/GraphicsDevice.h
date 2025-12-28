#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Config.h"

using Microsoft::WRL::ComPtr;

class GraphicsDevice
{
public:
    /**
     * @brief Default constructor for the GraphicsDevice class.
     */
    GraphicsDevice() = default;

    /**
     * @brief Destructor for the GraphicsDevice class.
     * Releases all DirectX 11 resources.
     */
    ~GraphicsDevice();

    // Initialization and shutdown
    /**
     * @brief Initializes the DirectX 11 device, context, swap chain, and main render targets.
     *
     * @param hwnd Handle to the window for the swap chain.
     * @return true if initialization succeeded, false otherwise.
     */
    bool Initialize(HWND hwnd);

    /**
     * @brief Releases all COM objects and cleans up the graphics device.
     */
    void Shutdown();

    // Frame operations
    /**
     * @brief Presents the back buffer to the screen.
     *
     * @param vsync Whether to wait for vertical sync.
     */
    void Present(bool vsync = true);

    /**
     * @brief Clears the back buffer with a specific color.
     *
     * @param color An array of 4 floats representing the RGBA color.
     */
    void ClearBackBuffer(const float *color);

    /**
     * @brief Clears the depth-stencil view.
     */
    void ClearDepthStencil();

    // Accessors
    /**
     * @brief Gets the ID3D11Device pointer.
     * @return Pointer to the D3D11 device.
     */
    [[nodiscard]] ID3D11Device *GetDevice() const
    {
        return m_device.Get();
    }

    /**
     * @brief Gets the ID3D11DeviceContext pointer.
     * @return Pointer to the D3D11 device context.
     */
    [[nodiscard]] ID3D11DeviceContext *GetContext() const
    {
        return m_context.Get();
    }

    /**
     * @brief Gets the IDXGISwapChain pointer.
     * @return Pointer to the DXGI swap chain.
     */
    [[nodiscard]] IDXGISwapChain *GetSwapChain() const
    {
        return m_swapChain.Get();
    }

    /**
     * @brief Gets the back buffer's render target view.
     * @return Pointer to the back buffer RTV.
     */
    [[nodiscard]] ID3D11RenderTargetView *GetBackBufferRTV() const
    {
        return m_backBufferRTV.Get();
    }

    /**
     * @brief Gets the main depth-stencil view.
     * @return Pointer to the depth-stencil view.
     */
    [[nodiscard]] ID3D11DepthStencilView *GetDepthStencilView() const
    {
        return m_depthStencilView.Get();
    }

    /**
     * @brief Gets the shader resource view of the depth buffer.
     * @return Pointer to the depth buffer SRV.
     */
    [[nodiscard]] ID3D11ShaderResourceView *GetDepthSRV() const
    {
        return m_depthSRV.Get();
    }

    // Utility methods
    /**
     * @brief Sets the back buffer as the current render target.
     */
    void SetBackBufferAsRenderTarget();

    /**
     * @brief Sets the viewport to the specified dimensions.
     *
     * @param width Width of the viewport in pixels.
     * @param height Height of the viewport in pixels.
     */
    void SetViewport(float width, float height);

    /**
     * @brief Sets the viewport to the default dimensions (back buffer size).
     */
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
