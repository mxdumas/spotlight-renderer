#pragma once

#include <cstdint>
#include <d3d11.h>
#include <string>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * @class Texture
 * @brief Manages the loading and usage of 2D textures.
 *
 * This class uses stb_image to load various image formats and creates a
 * DirectX 11 shader resource view for use in shaders.
 */
class Texture
{
public:
    /**
     * @brief Default constructor for the Texture class.
     */
    Texture();

    /**
     * @brief Destructor for the Texture class.
     * Releases the shader resource view.
     */
    ~Texture() = default;

    /**
     * @brief Loads a texture from an image file.
     *
     * @param device Pointer to the ID3D11Device.
     * @param fileName Path to the image file (e.g., .jpg, .png).
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadFromFile(ID3D11Device *device, const std::string &fileName);

    /**
     * @brief Loads a texture from memory (e.g. from a ZIP archive).
     *
     * @param device Pointer to the ID3D11Device.
     * @param data The raw file data (png/jpg bytes).
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadFromMemory(ID3D11Device *device, const std::vector<uint8_t> &data);

    /**
     * @brief Creates a Texture2DArray from multiple images.
     *
     * @param device Pointer to the ID3D11Device.
     * @param filesData List of raw file data buffers.
     * @return true if creation succeeded.
     */
    bool CreateTextureArray(ID3D11Device *device, const std::vector<std::vector<uint8_t>> &filesData);

    /**
     * @brief Gets the shader resource view of the texture.
     * @return Pointer to the ID3D11ShaderResourceView.
     */
    [[nodiscard]] ID3D11ShaderResourceView *GetSRV() const
    {
        return m_srv.Get();
    }

private:
    ComPtr<ID3D11ShaderResourceView> m_srv;
};
