#include "Shader.h"
#include <iostream>

Shader::Shader() = default;

bool Shader::LoadVertexShader(ID3D11Device *device, const std::wstring &fileName, const std::string &entryPoint,
                              const std::vector<D3D11_INPUT_ELEMENT_DESC> &inputElements)
{
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(),
                                    "vs_5_0", flags, 0, &shaderBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char *)errorBlob->GetBufferPointer());
        }
        return false;
    }

    hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr,
                                    &m_vertexShader);
    if (FAILED(hr))
        return false;

    if (!inputElements.empty())
    {
        hr = device->CreateInputLayout(inputElements.data(), (UINT)inputElements.size(), shaderBlob->GetBufferPointer(),
                                       shaderBlob->GetBufferSize(), &m_inputLayout);
        if (FAILED(hr))
            return false;
    }

    return true;
}

bool Shader::LoadPixelShader(ID3D11Device *device, const std::wstring &fileName, const std::string &entryPoint)
{
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(),
                                    "ps_5_0", flags, 0, &shaderBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char *)errorBlob->GetBufferPointer());
        }
        return false;
    }

    hr =
        device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_pixelShader);
    if (FAILED(hr))
        return false;

    return true;
}

bool Shader::LoadFromFile(ID3D11Device *device, const std::wstring &fileName,
                          const std::vector<D3D11_INPUT_ELEMENT_DESC> &inputElements)
{
    if (!LoadVertexShader(device, fileName, "VS", inputElements))
        return false;
    if (!LoadPixelShader(device, fileName, "PS"))
        return false;
    return true;
}

void Shader::Bind(ID3D11DeviceContext *context)
{
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}
