#include "Camera.h"
#include "GameObject.h"
#include "../Core/Graphics.h"
#include "../Input/InputManager.h"
#include "../Core/TimeManager.h"
#include <algorithm>

Camera::Camera(GameObject* owner)
    : Component(owner),
      m_graphics(nullptr),
      m_isEnabled(true),
      m_position(0.0f, 30.0f, -40.0f),
      m_pitch(30.0f),
      m_yaw(0.0f),
      m_fov(60.0f),
      m_nearZ(0.1f),
      m_farZ(2000.0f),
      m_moveSpeed(35.0f),
      m_mouseSensitivity(0.18f),
      m_lastMouseX(0),
      m_lastMouseY(0),
      m_isDragging(false)
{
    m_viewMatrix = DirectX::XMMatrixIdentity();
    m_projMatrix = DirectX::XMMatrixIdentity();
}

Camera::~Camera()
{
}

void Camera::Initialize(Graphics* graphics, float fov, float nearZ, float farZ)
{
    m_graphics = graphics;
    m_fov = fov;
    m_nearZ = nearZ;
    m_farZ = farZ;

    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera::Start()
{
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera::SetPosition(const DirectX::XMFLOAT3& pos)
{
    m_position = pos;
    UpdateViewMatrix();
}

void Camera::SetRotation(float pitch, float yaw)
{
    m_pitch = std::clamp(pitch, -89.0f, 89.0f);
    m_yaw = yaw;
    UpdateViewMatrix();
}

void Camera::Update()
{
    if (!m_isEnabled) return;

    InputManager* input = InputManager::GetInstance();
    TimeManager* time = TimeManager::GetInstance();
    if (!input || !time) return;

    float dt = time->GetDeltaTime();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.016f; // Clamp delta time in case of pauses

    // Mouse rotation with Right Click
    int curMouseX = input->GetMouseX();
    int curMouseY = input->GetMouseY();

    if (input->IsKeyPressed(VK_RBUTTON))
    {
        if (!m_isDragging)
        {
            m_isDragging = true;
            m_lastMouseX = curMouseX;
            m_lastMouseY = curMouseY;
        }
        else
        {
            float dx = static_cast<float>(curMouseX - m_lastMouseX);
            float dy = static_cast<float>(curMouseY - m_lastMouseY);

            m_yaw += dx * m_mouseSensitivity;
            m_pitch += dy * m_mouseSensitivity; // Natural pitch
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

            m_lastMouseX = curMouseX;
            m_lastMouseY = curMouseY;
        }
    }
    else
    {
        m_isDragging = false;
    }

    // Keyboard Movement
    float pitchRad = DirectX::XMConvertToRadians(m_pitch);
    float yawRad = DirectX::XMConvertToRadians(m_yaw);

    // Forward/Right vectors in XZ plane
    DirectX::XMVECTOR forwardVec = DirectX::XMVectorSet(
        sinf(yawRad) * cosf(pitchRad),
        -sinf(pitchRad),
        cosf(yawRad) * cosf(pitchRad),
        0.0f
    );
    forwardVec = DirectX::XMVector3Normalize(forwardVec);

    DirectX::XMVECTOR forwardFlat = DirectX::XMVectorSet(sinf(yawRad), 0.0f, cosf(yawRad), 0.0f);
    forwardFlat = DirectX::XMVector3Normalize(forwardFlat);

    DirectX::XMVECTOR rightVec = DirectX::XMVectorSet(cosf(yawRad), 0.0f, -sinf(yawRad), 0.0f);
    rightVec = DirectX::XMVector3Normalize(rightVec);

    DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    float speed = m_moveSpeed * dt;
    if (input->IsKeyPressed(VK_SHIFT))
    {
        speed *= 2.5f; // Sprint
    }

    DirectX::XMVECTOR moveDir = DirectX::XMVectorZero();

    if (input->IsKeyPressed('W')) moveDir = DirectX::XMVectorAdd(moveDir, DirectX::XMVectorScale(forwardVec, speed));
    if (input->IsKeyPressed('S')) moveDir = DirectX::XMVectorSubtract(moveDir, DirectX::XMVectorScale(forwardVec, speed));
    if (input->IsKeyPressed('D')) moveDir = DirectX::XMVectorAdd(moveDir, DirectX::XMVectorScale(rightVec, speed));
    if (input->IsKeyPressed('A')) moveDir = DirectX::XMVectorSubtract(moveDir, DirectX::XMVectorScale(rightVec, speed));
    if (input->IsKeyPressed('E') || input->IsKeyPressed(VK_SPACE)) moveDir = DirectX::XMVectorAdd(moveDir, DirectX::XMVectorScale(upVec, speed));
    if (input->IsKeyPressed('Q')) moveDir = DirectX::XMVectorSubtract(moveDir, DirectX::XMVectorScale(upVec, speed));

    DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&m_position);
    posVec = DirectX::XMVectorAdd(posVec, moveDir);
    DirectX::XMStoreFloat3(&m_position, posVec);

    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
    float pitchRad = DirectX::XMConvertToRadians(m_pitch);
    float yawRad = DirectX::XMConvertToRadians(m_yaw);

    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        sinf(yawRad) * cosf(pitchRad),
        -sinf(pitchRad),
        cosf(yawRad) * cosf(pitchRad),
        0.0f
    );
    forward = DirectX::XMVector3Normalize(forward);

    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&m_position);
    DirectX::XMVECTOR target = DirectX::XMVectorAdd(pos, forward);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    m_viewMatrix = DirectX::XMMatrixLookAtLH(pos, target, up);

    if (m_graphics)
    {
        m_graphics->SetViewMatrix(m_viewMatrix);
    }
}

void Camera::UpdateProjectionMatrix()
{
    // Aspect ratio 16:9 as default or 1280x720
    float aspect = 1280.0f / 720.0f;
    m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(m_fov),
        aspect,
        m_nearZ,
        m_farZ
    );

    if (m_graphics)
    {
        m_graphics->SetProjectionMatrix(m_projMatrix);
    }
}
