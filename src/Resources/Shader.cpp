#include "Shader.h"
#include <iostream>

Shader::Shader() = default;

bool Shader::LoadVertexShader(ID3D11Device *device, const std::wstring &file_name, const std::string &entry_point,
                              const std::vector<D3D11_INPUT_ELEMENT_DESC> &input_elements)
{
    ComPtr<ID3DBlob> shader_blob;
    ComPtr<ID3DBlob> error_blob;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile(file_name.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point.c_str(),
                                    "vs_5_0", flags, 0, &shader_blob, &error_blob);

    if (FAILED(hr))
    {
        if (error_blob)
        {
            OutputDebugStringA((char *)error_blob->GetBufferPointer());
        }
        return false;
    }

    hr = device->CreateVertexShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr,
                                    &m_vertexShader);
    if (FAILED(hr))
        return false;

    if (!input_elements.empty())
    {
        hr = device->CreateInputLayout(input_elements.data(), (UINT)input_elements.size(),
                                       shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), &m_inputLayout);
        if (FAILED(hr))
            return false;
    }

    return true;
}

bool Shader::LoadPixelShader(ID3D11Device *device, const std::wstring &file_name, const std::string &entry_point)
{
    ComPtr<ID3DBlob> shader_blob;
    ComPtr<ID3DBlob> error_blob;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile(file_name.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point.c_str(),
                                    "ps_5_0", flags, 0, &shader_blob, &error_blob);

    if (FAILED(hr))
    {
        if (error_blob)
        {
            OutputDebugStringA((char *)error_blob->GetBufferPointer());
        }
        return false;
    }

    hr = device->CreatePixelShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), nullptr,
                                   &m_pixelShader);
    if (FAILED(hr))
        return false;

    return true;
}

void Shader::Bind(ID3D11DeviceContext *context)
{
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}
