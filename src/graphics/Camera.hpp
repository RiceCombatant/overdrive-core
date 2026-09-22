#pragma once

#include <DirectXMath.h>
#include "core/MechController.hpp"

namespace Overdrive
{
    using namespace DirectX;

    enum class CameraMode
    {
        FPV, // First-Person Cockpit View (VR Target)
        TPS  // Third-Person Chase View (AC6 standard)
    };

    class Camera
    {
    public:
        Camera();

        void SetMode(CameraMode mode) { m_mode = mode; }
        void ToggleMode() { m_mode = (m_mode == CameraMode::FPV) ? CameraMode::TPS : CameraMode::FPV; }
        CameraMode GetMode() const { return m_mode; }

        void Update(float deltaTime, const MechController& mech);

        XMMATRIX GetViewMatrix() const;
        XMMATRIX GetProjectionMatrix(float aspectRatio) const;

        XMFLOAT3 GetEyePosition() const { return m_eyePos; }
        XMFLOAT3 GetLookTarget() const { return m_lookTarget; }

    private:
        CameraMode m_mode = CameraMode::TPS; // Default to TPS for immediate visual confirmation, toggleable to FPV

        XMFLOAT3 m_eyePos     = { 0.0f, 3.0f, -7.0f };
        XMFLOAT3 m_lookTarget = { 0.0f, 1.5f, 0.0f };
        XMFLOAT3 m_upVector   = { 0.0f, 1.0f, 0.0f };

        float m_fovY = XMConvertToRadians(65.0f);
        float m_nearZ = 0.1f;
        float m_farZ  = 1000.0f;
    };
}
