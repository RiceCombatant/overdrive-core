#include "Camera.hpp"
#include <cmath>

namespace Overdrive
{
    Camera::Camera()
    {
        m_nearZ = 0.05f; // Small near plane so geometry close to cockpit doesn't clip awkwardly
    }

    void Camera::Update(float deltaTime, const MechController& mech)
    {
        float yaw   = mech.GetYaw();
        float pitch = mech.GetPitch();
        float roll  = mech.GetRoll();

        float sinY = std::sin(yaw);
        float cosY = std::cos(yaw);
        float sinP = std::sin(pitch);
        float cosP = std::cos(pitch);

        // Forward look direction
        XMFLOAT3 forward = {
            sinY * cosP,
            -sinP,
            cosY * cosP
        };

        if (m_mode == CameraMode::FPV)
        {
            // First Person Cockpit View
            // Camera placed at front tip of cockpit/visor (Y=1.85m, 0.55m forward)
            XMFLOAT3 mechPos = mech.GetPosition();
            m_eyePos = {
                mechPos.x + sinY * 0.55f,
                mechPos.y + 1.85f,
                mechPos.z + cosY * 0.55f
            };

            // Look target 20m ahead along view direction
            m_lookTarget = {
                m_eyePos.x + forward.x * 20.0f,
                m_eyePos.y + forward.y * 20.0f,
                m_eyePos.z + forward.z * 20.0f
            };

            // Up vector stays stable upright with subtle cockpit tilt
            m_upVector = {
                -std::sin(roll * 0.4f) * cosY,
                std::cos(roll * 0.4f),
                std::sin(roll * 0.4f) * sinY
            };

            // Dynamic FOV for speed
            float targetFov = mech.IsAssaultBoost() ? XMConvertToRadians(76.0f) : XMConvertToRadians(68.0f);
            m_fovY += (targetFov - m_fovY) * 8.0f * deltaTime;
        }
        else
        {
            // Third Person Chase View (AC6 style)
            XMFLOAT3 target = mech.GetTPSLookTarget();
            m_lookTarget = target;

            // Camera placed further back to have a wide, comfortable view
            const float distance = 8.8f;       // Back distance (increased from 6.8)
            const float heightOffset = 2.8f;   // Height offset above ground

            // Ideal camera position behind the mech based on Yaw & Pitch
            XMFLOAT3 idealPos = {
                target.x - sinY * distance * cosP,
                target.y + heightOffset - forward.y * (distance * 0.4f),
                target.z - cosY * distance * cosP
            };

            // Smooth chase interpolation:
            // Moderate smoothing (6.0) prevents violent screen shakes during Quick Boost
            float smoothSpeed = mech.IsAssaultBoost() ? 10.0f : 6.0f;
            m_eyePos.x += (idealPos.x - m_eyePos.x) * smoothSpeed * deltaTime;
            m_eyePos.y += (idealPos.y - m_eyePos.y) * smoothSpeed * deltaTime;
            m_eyePos.z += (idealPos.z - m_eyePos.z) * smoothSpeed * deltaTime;

            // Up vector strictly world Y (prevents any camera roll shaking on side boosts)
            m_upVector = { 0.0f, 1.0f, 0.0f };

            float targetFov = mech.IsAssaultBoost() ? XMConvertToRadians(72.0f) : XMConvertToRadians(60.0f);
            m_fovY += (targetFov - m_fovY) * 6.0f * deltaTime;
        }
    }

    XMMATRIX Camera::GetViewMatrix() const
    {
        XMVECTOR eye    = XMLoadFloat3(&m_eyePos);
        XMVECTOR target = XMLoadFloat3(&m_lookTarget);
        XMVECTOR up     = XMLoadFloat3(&m_upVector);
        return XMMatrixLookAtLH(eye, target, up);
    }

    XMMATRIX Camera::GetProjectionMatrix(float aspectRatio) const
    {
        return XMMatrixPerspectiveFovLH(m_fovY, aspectRatio, m_nearZ, m_farZ);
    }
}
