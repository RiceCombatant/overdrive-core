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
    class AudioManager;
    class RemoteMech;
    class AIBotMech;

    struct LockTargetInfo
    {
        bool hasTarget = false;
        int targetIndex = -1;
        bool isRemoteMech = false;
        uint8_t remotePlayerId = 0; // 0 to 3
        bool isAIBot = false;
        uint8_t botId = 0;
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
            float yawDelta,
            float pitchDelta,
            AudioManager* audio = nullptr,
            const XMMATRIX* overrideViewProj = nullptr,
            const XMFLOAT3* overrideEyePos = nullptr,
            const std::vector<RemoteMech>* remoteMechs = nullptr,
            float lookStickX = 0.0f,
            float lookStickY = 0.0f,
            const std::vector<AIBotMech>* aiBots = nullptr);

        const LockTargetInfo& GetCurrentTarget() const { return m_targetInfo; }
        XMFLOAT2 GetInnerReticlePos() const { return m_currentReticleNdc; }
        XMFLOAT3 GetAimWorldTarget(const Camera& camera, const XMFLOAT3* overrideLookTarget = nullptr) const;

        void SetFcsModifiers(float closeAssist, float mediumAssist, float longAssist, float missileMod, float armTrackingRate = 1.0f);
        float GetCloseAssist() const { return m_fcsCloseAssist; }
        float GetMediumAssist() const { return m_fcsMediumAssist; }
        float GetLongAssist() const { return m_fcsLongAssist; }
        float GetMissileLockMod() const { return m_fcsMissileLockMod; }

    private:
        bool m_isHardLockEnabled = false;
        int m_lockedTargetIndex = -1;
        int m_prevLockedTargetIndex = -1;

        float m_targetSwitchCooldown = 0.0f;
        bool m_stickFlickTriggered = false;

        LockTargetInfo m_targetInfo;
        XMFLOAT2 m_currentReticleNdc = { 0.0f, 0.0f }; // Smoothly tracked inner reticle

        // FCS Distance categories & tracking speed multipliers
        // AC6 FCS tuning: Close (<130m), Medium (130m-260m), Long (>260m)
        float m_fcsCloseAssist = 1.0f;
        float m_fcsMediumAssist = 1.0f;
        float m_fcsLongAssist = 1.0f;
        float m_fcsMissileLockMod = 1.0f;
        float m_armTrackingRate = 1.0f;

        float CalculateFcsTrackingSpeed(float distance) const;
    };
}
