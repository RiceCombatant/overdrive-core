#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <DirectXMath.h>
#include <vector>
#include "combat/TargetDummy.hpp"
#include "graphics/Camera.hpp"

namespace Overdrive
{
    using namespace DirectX;

    class MechController;

    struct LockTargetInfo
    {
        bool hasTarget = false;
        int targetIndex = -1;
        XMFLOAT3 worldPos = { 0.0f, 0.0f, 0.0f };
        XMFLOAT2 screenNdc = { 0.0f, 0.0f }; // -1 to 1 in normalized device coords
        float distance = 0.0f;
        bool isAimed = false; // True when inner reticle converged on target
    };

    class TargetLockSystem
    {
    public:
        TargetLockSystem();

        void ToggleHardLock();
        void SetHardLock(bool enable);
        bool IsHardLockEnabled() const { return m_isHardLockEnabled; }

        void Update(
            float deltaTime,
            const Camera& camera,
            MechController& mech,
            const std::vector<TargetDummy>& targets,
            float mouseDeltaLen);

        const LockTargetInfo& GetCurrentTarget() const { return m_targetInfo; }
        XMFLOAT2 GetInnerReticlePos() const { return m_currentReticleNdc; }
        XMFLOAT3 GetAimWorldTarget(const Camera& camera) const;

    private:
        bool m_isHardLockEnabled = false;
        int m_lockedTargetIndex = -1;

        LockTargetInfo m_targetInfo;
        XMFLOAT2 m_currentReticleNdc = { 0.0f, 0.0f }; // Smoothly tracked inner reticle

        // FCS Distance categories & tracking speed multipliers
        // AC6 FCS tuning: Close (<130m), Medium (130m-260m), Long (>260m)
        float CalculateFcsTrackingSpeed(float distance) const;
    };
}
