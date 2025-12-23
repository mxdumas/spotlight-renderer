#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Texture {
public:
    Texture();
    ~Texture();

    bool LoadFromFile(ID3D11Device* device, const std::string& fileName);
    ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

private:
    ComPtr<ID3D11ShaderResourceView> m_srv;
};
