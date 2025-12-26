#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
#include "stb_image.h"

Texture::Texture() = default;

bool Texture::LoadFromFile(ID3D11Device *device, const std::string &fileName)
{
    int width, height, channels;
    unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &channels, 4);

    bool isProcedural = false;
    if (!data)
    {
        // Create a procedural circle gobo as fallback
        width = 512;
        height = 512;
        data = (unsigned char *)malloc(width * height * 4);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                float dx =
                    (static_cast<float>(x) - static_cast<float>(width) / 2.0f) / (static_cast<float>(width) / 2.0f);
                float dy =
                    (static_cast<float>(y) - static_cast<float>(height) / 2.0f) / (static_cast<float>(height) / 2.0f);
                float dist = sqrtf((dx * dx) + (dy * dy));
                unsigned char val = (dist < 0.8f) ? 255 : 0;
                int idx = (y * width + x) * 4;
                data[idx] = data[idx + 1] = data[idx + 2] = val;
                data[idx + 3] = 255;
            }
        }
        isProcedural = true;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData = {};
    subData.pSysMem = data;
    subData.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device->CreateTexture2D(&desc, &subData, &texture);

    if (isProcedural)
        free(data);
    else
        stbi_image_free(data);

    if (FAILED(hr))
        return false;

    hr = device->CreateShaderResourceView(texture.Get(), nullptr, &m_srv);
    return SUCCEEDED(hr);
}

bool Texture::LoadFromMemory(ID3D11Device *device, const std::vector<uint8_t> &data)
{
    int width, height, channels;
    unsigned char *pixels = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);

    if (!pixels)
        return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData = {};
    subData.pSysMem = pixels;
    subData.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device->CreateTexture2D(&desc, &subData, &texture);
    stbi_image_free(pixels);

    if (FAILED(hr))
        return false;

    hr = device->CreateShaderResourceView(texture.Get(), nullptr, &m_srv);
    return SUCCEEDED(hr);
}

bool Texture::CreateTextureArray(ID3D11Device *device, const std::vector<std::vector<uint8_t>> &filesData)
{
    if (filesData.empty())
        return false;

    // Load all images and determine max dimensions
    struct ImageData {
        unsigned char *pixels;
        int width;
        int height;
    };
    std::vector<ImageData> images;
    int max_width = 0, max_height = 0;

    for (const auto &file_data : filesData)
    {
        int w, h, c;
        unsigned char *pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()), &w, &h, &c, 4);
        if (pixels)
        {
            // Convert transparent pixels to black (gobo mask)
            for (int i = 0; i < w * h; ++i)
            {
                int idx = i * 4;
                if (pixels[idx + 3] < 128) // Alpha < 50%
                {
                    pixels[idx] = 0;     // R
                    pixels[idx + 1] = 0; // G
                    pixels[idx + 2] = 0; // B
                }
            }
            images.push_back({pixels, w, h});
            if (w > max_width) max_width = w;
            if (h > max_height) max_height = h;
        }
    }

    if (images.empty())
        return false;

    // Create texture array
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = max_width;
    desc.Height = max_height;
    desc.MipLevels = 1;
    desc.ArraySize = static_cast<UINT>(images.size());
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Prepare subresource data for each slice
    std::vector<D3D11_SUBRESOURCE_DATA> subDatas(images.size());
    std::vector<std::vector<unsigned char>> resizedImages(images.size());

    for (size_t i = 0; i < images.size(); ++i)
    {
        // If image is smaller than max, we need to resize/pad it
        if (images[i].width != max_width || images[i].height != max_height)
        {
            resizedImages[i].resize(max_width * max_height * 4, 0);
            // Copy centered
            int offset_x = (max_width - images[i].width) / 2;
            int offset_y = (max_height - images[i].height) / 2;
            for (int y = 0; y < images[i].height; ++y)
            {
                for (int x = 0; x < images[i].width; ++x)
                {
                    int src_idx = (y * images[i].width + x) * 4;
                    int dst_idx = ((y + offset_y) * max_width + (x + offset_x)) * 4;
                    resizedImages[i][dst_idx] = images[i].pixels[src_idx];
                    resizedImages[i][dst_idx + 1] = images[i].pixels[src_idx + 1];
                    resizedImages[i][dst_idx + 2] = images[i].pixels[src_idx + 2];
                    resizedImages[i][dst_idx + 3] = images[i].pixels[src_idx + 3];
                }
            }
            subDatas[i].pSysMem = resizedImages[i].data();
        }
        else
        {
            subDatas[i].pSysMem = images[i].pixels;
        }
        subDatas[i].SysMemPitch = max_width * 4;
        subDatas[i].SysMemSlicePitch = 0;
    }

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device->CreateTexture2D(&desc, subDatas.data(), &texture);

    // Free loaded images
    for (auto &img : images)
        stbi_image_free(img.pixels);

    if (FAILED(hr))
        return false;

    // Create SRV for texture array
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(images.size());

    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, &m_srv);
    return SUCCEEDED(hr);
}
