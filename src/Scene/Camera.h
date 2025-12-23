#pragma once

#include <DirectXMath.h>

/**
 * @class Camera
 * @brief Simple camera class for managing view and projection matrices.
 */
class Camera
{
public:
    /**
     * @brief Default constructor for the Camera class.
     * Initializes view and projection matrices to identity.
     */
    Camera();

    /**
     * @brief Sets the camera's view matrix using eye, target, and up vectors.
     *
     * @param eye The position of the camera.
     * @param at The point the camera is looking at.
     * @param up The up vector (usually 0, 1, 0).
     */
    void SetLookAt(DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up);

    /**
     * @brief Sets the camera's perspective projection matrix.
     *
     * @param fovY Vertical field of view in radians.
     * @param aspect Aspect ratio (width / height).
     * @param nearZ Near clipping plane distance.
     * @param farZ Far clipping plane distance.
     */
    void SetPerspective(float fovY, float aspect, float nearZ, float farZ);

    /**
     * @brief Gets the current view matrix.
     * @return 4x4 view matrix.
     */
    DirectX::XMMATRIX GetViewMatrix() const
    {
        return DirectX::XMLoadFloat4x4(&m_view);
    }

    /**
     * @brief Gets the current projection matrix.
     * @return 4x4 projection matrix.
     */
    DirectX::XMMATRIX GetProjectionMatrix() const
    {
        return DirectX::XMLoadFloat4x4(&m_projection);
    }

private:
    DirectX::XMFLOAT4X4 m_view;
    DirectX::XMFLOAT4X4 m_projection;
};
