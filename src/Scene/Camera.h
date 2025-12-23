#pragma once

#include <DirectXMath.h>

class Camera {
public:
    Camera();
    void SetLookAt(DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up);
    void SetPerspective(float fovY, float aspect, float nearZ, float farZ);

    DirectX::XMMATRIX GetViewMatrix() const { return DirectX::XMLoadFloat4x4(&m_view); }
    DirectX::XMMATRIX GetProjectionMatrix() const { return DirectX::XMLoadFloat4x4(&m_projection); }

private:
    DirectX::XMFLOAT4X4 m_view;
    DirectX::XMFLOAT4X4 m_projection;
};
