#pragma once
#include "Component.h"
#include <DirectXMath.h>

class Graphics;

class Camera : public Component
{
public:
    Camera(GameObject* owner);
    virtual ~Camera();

    void Initialize(Graphics* graphics, float fov = 60.0f, float nearZ = 0.1f, float farZ = 2000.0f);

    virtual void Start() override;
    virtual void Update() override;

    void SetPosition(const DirectX::XMFLOAT3& pos);
    void SetRotation(float pitch, float yaw);

    const DirectX::XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }
    const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_projMatrix; }

    DirectX::XMFLOAT3 GetPosition() const { return m_position; }
    float GetPitch() const { return m_pitch; }
    float GetYaw() const { return m_yaw; }

    void SetEnabled(bool enabled) { m_isEnabled = enabled; }
    bool IsEnabled() const { return m_isEnabled; }

private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    Graphics* m_graphics;
    bool m_isEnabled;

    DirectX::XMFLOAT3 m_position;
    float m_pitch; // degrees
    float m_yaw;   // degrees

    float m_fov;
    float m_nearZ;
    float m_farZ;
    float m_moveSpeed;
    float m_mouseSensitivity;

    int m_lastMouseX;
    int m_lastMouseY;
    bool m_isDragging;

    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projMatrix;
};
