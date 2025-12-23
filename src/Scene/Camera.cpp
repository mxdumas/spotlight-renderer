#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
    XMStoreFloat4x4(&m_view, XMMatrixIdentity());
    XMStoreFloat4x4(&m_projection, XMMatrixIdentity());
}

void Camera::SetLookAt(XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up)
{
    XMVECTOR eyeVec = XMLoadFloat3(&eye);
    XMVECTOR atVec = XMLoadFloat3(&at);
    XMVECTOR upVec = XMLoadFloat3(&up);
    XMStoreFloat4x4(&m_view, XMMatrixLookAtLH(eyeVec, atVec, upVec));
}

void Camera::SetPerspective(float fovY, float aspect, float nearZ, float farZ)
{
    XMStoreFloat4x4(&m_projection, XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ));
}
