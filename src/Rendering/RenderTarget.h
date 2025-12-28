#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * @class RenderTarget
 * @brief Encapsulates a DirectX 11 render target, including its texture, RTV, and SRV.
 *
 * This class simplifies the creation and management of off-screen buffers used for
 * intermediate rendering steps like shadow mapping or post-processing.
 */
class RenderTarget
{
public:
    /**
     * @brief Default constructor for the RenderTarget class.
     */
    RenderTarget() = default;

    /**
     * @brief Destructor for the RenderTarget class.
     * Releases all associated GPU resources.
     */
    ~RenderTarget();

    /// @name Copy/Move semantics
    /// @{
    RenderTarget(const RenderTarget &) = delete;
    RenderTarget &operator=(const RenderTarget &) = delete;
    /// @brief Move constructor (transfers ownership of GPU resources).
    RenderTarget(RenderTarget &&) noexcept = default;
    /// @brief Move assignment operator (transfers ownership of GPU resources).
    RenderTarget &operator=(RenderTarget &&) noexcept = default;
    /// @}

    /**
     * @brief Creates the render target texture and its corresponding views.
     *
     * @param device Pointer to the ID3D11Device.
     * @param width Width of the render target in pixels.
     * @param height Height of the render target in pixels.
     * @param format The texture format (default is RGBA8 UNORM).
     * @return true if creation succeeded, false otherwise.
     */
    bool Create(ID3D11Device *device, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

    /**
     * @brief Shuts down the render target and releases all resources.
     */
    void Shutdown();

    /**
     * @brief Binds this render target to the pipeline.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param dsv Optional pointer to a depth-stencil view. If null, no depth buffer is bound.
     */
    void Bind(ID3D11DeviceContext *context, ID3D11DepthStencilView *dsv = nullptr);

    /**
     * @brief Clears the render target with a specific color.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     * @param color An array of 4 floats representing the RGBA color.
     */
    void Clear(ID3D11DeviceContext *context, const float *color);

    /**
     * @brief Gets the underlying ID3D11Texture2D pointer.
     * @return Pointer to the texture.
     */
    [[nodiscard]] ID3D11Texture2D *GetTexture() const
    {
        return m_texture.Get();
    }

    /**
     * @brief Gets the render target view (RTV).
     * @return Pointer to the RTV.
     */
    [[nodiscard]] ID3D11RenderTargetView *GetRTV() const
    {
        return m_rtv.Get();
    }

    /**
     * @brief Gets the shader resource view (SRV) for sampling this target in a shader.
     * @return Pointer to the SRV.
     */
    [[nodiscard]] ID3D11ShaderResourceView *GetSRV() const
    {
        return m_srv.Get();
    }

    /**
     * @brief Gets the address of the RTV pointer.
     * @return Address of the RTV pointer.
     */
    [[nodiscard]] ID3D11RenderTargetView *const *GetRTVAddressOf() const
    {
        return m_rtv.GetAddressOf();
    }

    /**
     * @brief Gets the address of the SRV pointer.
     * @return Address of the SRV pointer.
     */
    [[nodiscard]] ID3D11ShaderResourceView *const *GetSRVAddressOf() const
    {
        return m_srv.GetAddressOf();
    }

    /**
     * @brief Gets the width of the render target.
     * @return Width in pixels.
     */
    [[nodiscard]] int GetWidth() const
    {
        return m_width;
    }

    /**
     * @brief Gets the height of the render target.
     * @return Height in pixels.
     */
    [[nodiscard]] int GetHeight() const
    {
        return m_height;
    }

private:
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11RenderTargetView> m_rtv;
    ComPtr<ID3D11ShaderResourceView> m_srv;
    int m_width = 0;
    int m_height = 0;
};
