#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
    XMStoreFloat4x4(&m_view, XMMatrixIdentity());
    XMStoreFloat4x4(&m_projection, XMMatrixIdentity());
}

void Camera::SetLookAt(XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up)
{
    XMVECTOR eye_vec = XMLoadFloat3(&eye);
    XMVECTOR at_vec = XMLoadFloat3(&at);
    XMVECTOR up_vec = XMLoadFloat3(&up);
    XMStoreFloat4x4(&m_view, XMMatrixLookAtLH(eye_vec, at_vec, up_vec));
}

void Camera::SetPerspective(float fov_y, float aspect, float near_z, float far_z)
{
    XMStoreFloat4x4(&m_projection, XMMatrixPerspectiveFovLH(fov_y, aspect, near_z, far_z));
}
