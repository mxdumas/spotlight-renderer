#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * @class Shader
 * @brief Manages the compilation, loading, and binding of vertex and pixel shaders.
 *
 * This class encapsulates DirectX 11 shader objects and their corresponding input layout.
 */
class Shader
{
public:
    /**
     * @brief Default constructor for the Shader class.
     */
    Shader();

    /**
     * @brief Destructor for the Shader class.
     * Releases compiled shader objects and input layout.
     */
    ~Shader() = default;

    /**
     * @brief Compiles and loads a vertex shader from a file.
     *
     * @param device Pointer to the ID3D11Device.
     * @param file_name Wide-string path to the .hlsl file.
     * @param entry_point Name of the entry point function in the shader file.
     * @param input_elements Description of the vertex input layout.
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadVertexShader(ID3D11Device *device, const std::wstring &file_name, const std::string &entry_point,
                          const std::vector<D3D11_INPUT_ELEMENT_DESC> &input_elements);

    /**
     * @brief Compiles and loads a pixel shader from a file.
     *
     * @param device Pointer to the ID3D11Device.
     * @param file_name Wide-string path to the .hlsl file.
     * @param entry_point Name of the entry point function in the shader file.
     * @return true if loading succeeded, false otherwise.
     */
    bool LoadPixelShader(ID3D11Device *device, const std::wstring &file_name, const std::string &entry_point);

    /**
     * @brief Binds the vertex shader, pixel shader, and input layout to the pipeline.
     *
     * @param context Pointer to the ID3D11DeviceContext.
     */
    void Bind(ID3D11DeviceContext *context);

    /**
     * @brief Gets the internal ID3D11VertexShader pointer.
     * @return Pointer to the vertex shader.
     */
    ID3D11VertexShader *GetVertexShader()
    {
        return m_vertexShader.Get();
    }

    /**
     * @brief Gets the internal ID3D11PixelShader pointer.
     * @return Pointer to the pixel shader.
     */
    ID3D11PixelShader *GetPixelShader()
    {
        return m_pixelShader.Get();
    }

    /**
     * @brief Gets the internal ID3D11InputLayout pointer.
     * @return Pointer to the input layout.
     */
    ID3D11InputLayout *GetInputLayout()
    {
        return m_inputLayout.Get();
    }

private:
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
};
