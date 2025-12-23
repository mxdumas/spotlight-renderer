#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"

Texture::Texture() {}
Texture::~Texture() {}

bool Texture::LoadFromFile(ID3D11Device* device, const std::string& fileName) {
    int width, height, channels;
    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &channels, 4);
    
    bool isProcedural = false;
    if (!data) {
        // Create a procedural circle gobo as fallback
        width = 512;
        height = 512;
        data = (unsigned char*)malloc(width * height * 4);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = (x - width / 2.0f) / (width / 2.0f);
                float dy = (y - height / 2.0f) / (height / 2.0f);
                float dist = sqrtf(dx*dx + dy*dy);
                unsigned char val = (dist < 0.8f) ? 255 : 0;
                int idx = (y * width + x) * 4;
                data[idx] = data[idx+1] = data[idx+2] = val;
                data[idx+3] = 255;
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
    
    if (isProcedural) free(data);
    else stbi_image_free(data);

    if (FAILED(hr)) return false;

    hr = device->CreateShaderResourceView(texture.Get(), nullptr, &m_srv);
    return SUCCEEDED(hr);
}
