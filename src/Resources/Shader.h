#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

class Shader {
public:
    Shader();
    ~Shader();

    bool LoadVertexShader(ID3D11Device* device, const std::wstring& fileName, const std::string& entryPoint, const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElements);
    bool LoadPixelShader(ID3D11Device* device, const std::wstring& fileName, const std::string& entryPoint);

    void Bind(ID3D11DeviceContext* context);

    ID3D11VertexShader* GetVertexShader() { return m_vertexShader.Get(); }
    ID3D11PixelShader* GetPixelShader() { return m_pixelShader.Get(); }
    ID3D11InputLayout* GetInputLayout() { return m_inputLayout.Get(); }

private:
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
};
