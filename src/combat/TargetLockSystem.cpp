#include "combat/TargetLockSystem.hpp"
#include "core/MechController.hpp"
#include "audio/AudioManager.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Overdrive
{
    TargetLockSystem::TargetLockSystem()
    {
    }

    void TargetLockSystem::ToggleHardLock()
    {
        m_isHardLockEnabled = !m_isHardLockEnabled;
        if (!m_isHardLockEnabled)
        {
            m_lockedTargetIndex = -1;
        }
        std::cout << "[TARGET ASSIST] " << (m_isHardLockEnabled ? "ENABLED (Hard Lock-on)" : "DISABLED (Manual Camera)") << std::endl;
    }

    void TargetLockSystem::SetHardLock(bool enable)
    {
        m_isHardLockEnabled = enable;
        if (!m_isHardLockEnabled)
        {
            m_lockedTargetIndex = -1;
        }
    }

    float TargetLockSystem::CalculateFcsTrackingSpeed(float distance) const
    {
        // AC6 FCS tuning characteristics:
        // Close-range (< 130m): Very quick tracking & lock acquisition
        // Medium-range (130m - 260m): Balanced tracking
        // Long-range (> 260m): Slower reticle convergence
        if (distance < 130.0f)
        {
            return 18.0f;
        }
        else if (distance < 260.0f)
        {
            return 12.0f;
        }
        else
        {
            return 7.0f;
        }
    }

    void TargetLockSystem::Update(
        float deltaTime,
        const Camera& camera,
        MechController& mech,
        const std::vector<TargetDummy>& targets,
        float mouseDeltaLen,
        AudioManager* audio)
    {
        if (deltaTime <= 0.0f) return;

        // AC6 Mechanic: Intentional large flick/swipe of mouse disengages Target Assist
        // Raised threshold to 0.45f so normal movement/aiming doesn't accidentally cancel Hard-Lock
        if (m_isHardLockEnabled && mouseDeltaLen > 0.45f)
        {
            m_isHardLockEnabled = false;
            m_lockedTargetIndex = -1;
            std::cout << "[TARGET ASSIST] Disengaged due to manual camera flick" << std::endl;
        }

        // 1. Calculate View-Projection Matrix
        // Standard aspect ratio for projection
        XMMATRIX viewMat = camera.GetViewMatrix();
        XMMATRIX projMat = camera.GetProjectionMatrix(16.0f / 9.0f);
        XMMATRIX viewProj = XMMatrixMultiply(viewMat, projMat);

        XMFLOAT3 mechPos = mech.GetPosition();

        // 2. Find best target in view
        int bestIdx = -1;
        float bestScore = 999999.0f;
        XMFLOAT2 bestNdc = { 0.0f, 0.0f };
        XMFLOAT3 bestWorldPos = { 0.0f, 0.0f, 0.0f };
        float bestDist = 0.0f;

        for (int i = 0; i < static_cast<int>(targets.size()); ++i)
        {
            const auto& t = targets[i];
            if (!t.IsAlive()) continue;

            XMFLOAT3 pos = t.GetPosition();
            pos.y += 0.2f; // Aim at core body

            // Transform target position into Clip / NDC coordinates
            XMVECTOR worldVec = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
            XMVECTOR clipVec  = XMVector4Transform(worldVec, viewProj);
            XMFLOAT4 clip;
            XMStoreFloat4(&clip, clipVec);

            // Must be in front of the camera
            if (clip.w <= 0.1f) continue;

            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;
            float ndcZ = clip.z / clip.w;

            // Must be within near/far planes
            if (ndcZ < 0.0f || ndcZ > 1.0f) continue;

            float screenDist = std::sqrt(ndcX * ndcX + ndcY * ndcY);

            // Calculate 3D distance to mech
            float dx = pos.x - mechPos.x;
            float dy = pos.y - mechPos.y;
            float dz = pos.z - mechPos.z;
            float worldDist = std::sqrt(dx * dx + dy * dy + dz * dz);

            // FCS effective detection radius:
            // For already-locked targets in Hard-Lock mode, allow a generous boundary (1.40)
            // so aggressive QB dashes or vertical ascents won't lose lock!
            float maxScreenDist = (m_isHardLockEnabled && m_lockedTargetIndex == i) ? 1.40f : 0.85f;
            if (screenDist < maxScreenDist && worldDist < 450.0f)
            {
                // If Hard Lock is enabled and this was already our locked target, strongly prioritize it
                float bonus = (m_isHardLockEnabled && m_lockedTargetIndex == i) ? -800.0f : 0.0f;
                float score = screenDist * 120.0f + worldDist + bonus;

                if (score < bestScore)
                {
                    bestScore = score;
                    bestIdx = i;
                    bestNdc = { ndcX, ndcY };
                    bestWorldPos = pos;
                    bestDist = worldDist;
                }
            }
        }

        // 3. Update Lock Target state
        if (bestIdx >= 0)
        {
            if (audio && bestIdx != m_prevLockedTargetIndex)
            {
                audio->PlayLockOn();
            }
            m_prevLockedTargetIndex = bestIdx;

            m_targetInfo.hasTarget = true;
            m_targetInfo.targetIndex = bestIdx;
            m_targetInfo.worldPos = bestWorldPos;
            m_targetInfo.screenNdc = bestNdc;
            m_targetInfo.distance = bestDist;

            if (m_isHardLockEnabled)
            {
                m_lockedTargetIndex = bestIdx;
                // Auto-track camera / mech orientation towards target in Hard-Lock mode (Image 2)
                mech.TrackTarget(bestWorldPos, deltaTime, 12.0f);
            }

            // 4. Smoothly interpolate inner reticle towards target NDC (Image 1 FCS Assist)
            float trackSpeed = CalculateFcsTrackingSpeed(bestDist);
            float tStep = std::clamp(trackSpeed * deltaTime, 0.0f, 1.0f);

            m_currentReticleNdc.x += (bestNdc.x - m_currentReticleNdc.x) * tStep;
            m_currentReticleNdc.y += (bestNdc.y - m_currentReticleNdc.y) * tStep;

            float diffX = bestNdc.x - m_currentReticleNdc.x;
            float diffY = bestNdc.y - m_currentReticleNdc.y;
            m_targetInfo.isAimed = (std::sqrt(diffX * diffX + diffY * diffY) < 0.08f);
        }
        else
        {
            m_targetInfo.hasTarget = false;
            m_targetInfo.targetIndex = -1;
            m_targetInfo.isAimed = false;

            // Reticle smoothly returns to center when no target
            float returnStep = std::clamp(10.0f * deltaTime, 0.0f, 1.0f);
            m_currentReticleNdc.x += (0.0f - m_currentReticleNdc.x) * returnStep;
            m_currentReticleNdc.y += (0.0f - m_currentReticleNdc.y) * returnStep;

            if (m_isHardLockEnabled)
            {
                m_lockedTargetIndex = -1;
            }
            m_prevLockedTargetIndex = -1;
        }
    }

    XMFLOAT3 TargetLockSystem::GetAimWorldTarget(const Camera& camera) const
    {
        if (m_targetInfo.hasTarget)
        {
            return m_targetInfo.worldPos;
        }
        return camera.GetLookTarget();
    }
}
