#include "combat/TargetLockSystem.hpp"
#include "combat/AIBotMech.hpp"
#include "core/MechController.hpp"
#include "audio/AudioManager.hpp"
#include "network/NetworkManager.hpp"
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

    void TargetLockSystem::SetFcsModifiers(
        float closeAssist,
        float mediumAssist,
        float longAssist,
        float missileMod,
        float armTrackingRate
    )
    {
        m_fcsCloseAssist = closeAssist;
        m_fcsMediumAssist = mediumAssist;
        m_fcsLongAssist = longAssist;
        m_fcsMissileLockMod = missileMod;
        m_armTrackingRate = armTrackingRate;
    }

    float TargetLockSystem::CalculateFcsTrackingSpeed(float distance) const
    {
        // AC6 FCS tuning characteristics:
        // Close-range (< 130m): Scaled by FCS Close-Range Assist
        // Medium-range (130m - 260m): Scaled by FCS Medium-Range Assist
        // Long-range (> 260m): Scaled by FCS Long-Range Assist
        // Overall rate is also influenced by Arm Firearm Servo Tracking Rate
        float baseRate = 12.0f;
        if (distance < 130.0f)
        {
            baseRate = 18.0f * m_fcsCloseAssist;
        }
        else if (distance < 260.0f)
        {
            baseRate = 12.0f * m_fcsMediumAssist;
        }
        else
        {
            baseRate = 7.0f * m_fcsLongAssist;
        }

        return baseRate * m_armTrackingRate;
    }

    void TargetLockSystem::Update(
        float deltaTime,
        const Camera& camera,
        MechController& mech,
        const std::vector<TargetDummy>& targets,
        float yawDelta,
        float pitchDelta,
        AudioManager* audio,
        const XMMATRIX* overrideViewProj,
        const XMFLOAT3* overrideEyePos,
        const std::vector<RemoteMech>* remoteMechs,
        float lookStickX,
        float lookStickY,
        const std::vector<AIBotMech>* aiBots)
    {
        if (deltaTime <= 0.0f) return;

        float mouseDeltaLen = std::sqrt(yawDelta * yawDelta + pitchDelta * pitchDelta);

        // 1. Calculate View-Projection Matrix
        XMMATRIX viewProj;
        if (overrideViewProj)
        {
            viewProj = *overrideViewProj;
        }
        else
        {
            XMMATRIX viewMat = camera.GetViewMatrix();
            XMMATRIX projMat = camera.GetProjectionMatrix(16.0f / 9.0f);
            viewProj = XMMatrixMultiply(viewMat, projMat);
        }

        XMFLOAT3 mechPos = overrideEyePos ? *overrideEyePos : mech.GetPosition();

        // 2. Gather all visible alive target candidates
        struct TargetCandidate {
            int idx;
            XMFLOAT2 ndc;
            XMFLOAT3 worldPos;
            float worldDist;
            float screenDist;
            bool isRemoteMech;
            uint8_t remotePlayerId;
            bool isAIBot;
            uint8_t botId;
        };
        std::vector<TargetCandidate> candidates;

        // Check RemoteMechs (Enemy players) first!
        if (remoteMechs)
        {
            for (const auto& rMech : *remoteMechs)
            {
                if (!rMech.IsAlive()) continue;

                XMFLOAT3 pos = rMech.GetPosition();
                pos.y += 0.8f; // Aim at core center

                XMVECTOR worldVec = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
                XMVECTOR clipVec  = XMVector4Transform(worldVec, viewProj);
                XMFLOAT4 clip;
                XMStoreFloat4(&clip, clipVec);

                if (clip.w > 0.1f)
                {
                    float ndcX = clip.x / clip.w;
                    float ndcY = clip.y / clip.w;
                    float ndcZ = clip.z / clip.w;

                    if (ndcZ >= 0.0f && ndcZ <= 1.0f)
                    {
                        float screenDist = std::sqrt(ndcX * ndcX + ndcY * ndcY);
                        float dx = pos.x - mechPos.x;
                        float dy = pos.y - mechPos.y;
                        float dz = pos.z - mechPos.z;
                        float worldDist = std::sqrt(dx * dx + dy * dy + dz * dz);

                        candidates.push_back({ 1000 + static_cast<int>(rMech.GetPlayerId()), { ndcX, ndcY }, pos, worldDist, screenDist, true, rMech.GetPlayerId(), false, 0 });
                    }
                }
            }
        }

        // Check AI Combat Bots
        if (aiBots)
        {
            for (const auto& bot : *aiBots)
            {
                if (!bot.IsAlive()) continue;

                XMFLOAT3 pos = bot.GetPosition();
                pos.y += 0.8f; // Aim at core center

                XMVECTOR worldVec = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
                XMVECTOR clipVec  = XMVector4Transform(worldVec, viewProj);
                XMFLOAT4 clip;
                XMStoreFloat4(&clip, clipVec);

                if (clip.w > 0.1f)
                {
                    float ndcX = clip.x / clip.w;
                    float ndcY = clip.y / clip.w;
                    float ndcZ = clip.z / clip.w;

                    if (ndcZ >= 0.0f && ndcZ <= 1.0f)
                    {
                        float screenDist = std::sqrt(ndcX * ndcX + ndcY * ndcY);
                        float dx = pos.x - mechPos.x;
                        float dy = pos.y - mechPos.y;
                        float dz = pos.z - mechPos.z;
                        float worldDist = std::sqrt(dx * dx + dy * dy + dz * dz);

                        candidates.push_back({ 2000 + static_cast<int>(bot.GetBotId()), { ndcX, ndcY }, pos, worldDist, screenDist, false, 0, true, bot.GetBotId() });
                    }
                }
            }
        }

        for (int i = 0; i < static_cast<int>(targets.size()); ++i)
        {
            const auto& t = targets[i];
            if (!t.IsAlive()) continue;

            XMFLOAT3 pos = t.GetPosition();
            pos.y += 0.2f; // Aim at core body

            XMVECTOR worldVec = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
            XMVECTOR clipVec  = XMVector4Transform(worldVec, viewProj);
            XMFLOAT4 clip;
            XMStoreFloat4(&clip, clipVec);

            if (clip.w <= 0.1f) continue;

            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;
            float ndcZ = clip.z / clip.w;

            if (ndcZ < 0.0f || ndcZ > 1.0f) continue;

            float screenDist = std::sqrt(ndcX * ndcX + ndcY * ndcY);

            float dx = pos.x - mechPos.x;
            float dy = pos.y - mechPos.y;
            float dz = pos.z - mechPos.z;
            float worldDist = std::sqrt(dx * dx + dy * dy + dz * dz);

            candidates.push_back({ i, { ndcX, ndcY }, pos, worldDist, screenDist, false, 0, false, 0 });
        }

        // AC6 Target Switching Mechanic:
        // Support both Gamepad Right-Stick Flick and Mouse Swipe/Flick!
        if (m_targetSwitchCooldown > 0.0f)
        {
            m_targetSwitchCooldown -= deltaTime;
        }

        float stickMag = std::sqrt(lookStickX * lookStickX + lookStickY * lookStickY);
        bool stickFlick = false;
        float flickDirX = 0.0f;
        float flickDirY = 0.0f;

        // 1. Controller Right-Stick Flick Detection:
        // Triggered on stick deflection (> 0.45) from neutral with debounce cooldown
        if (stickMag > 0.45f)
        {
            if (!m_stickFlickTriggered && m_targetSwitchCooldown <= 0.0f)
            {
                stickFlick = true;
                m_stickFlickTriggered = true;
                m_targetSwitchCooldown = 0.28f;
                flickDirX = lookStickX / stickMag;
                flickDirY = -lookStickY / stickMag; // SDL stick Up (-Y) maps to Screen Up (+Y)
            }
        }
        else if (stickMag < 0.25f)
        {
            m_stickFlickTriggered = false; // Reset trigger when stick returns near center
        }

        // 2. Mouse Fast Flick / Swipe Detection:
        bool mouseFlick = false;
        if (stickMag <= 0.15f && mouseDeltaLen > 0.038f && m_targetSwitchCooldown <= 0.0f)
        {
            mouseFlick = true;
            m_targetSwitchCooldown = 0.22f;
            flickDirX = yawDelta / mouseDeltaLen;
            flickDirY = -pitchDelta / mouseDeltaLen;
        }

        if (m_isHardLockEnabled && m_lockedTargetIndex >= 0 && (stickFlick || mouseFlick))
        {
            XMFLOAT2 currentNdc = { 0.0f, 0.0f };
            for (const auto& c : candidates)
            {
                if (c.idx == m_lockedTargetIndex)
                {
                    currentNdc = c.ndc;
                    break;
                }
            }

            int switchIdx = -1;
            float bestDot = 0.10f; // Must be in the general flick direction (> ~85 degrees)
            float bestScore = 999999.0f;

            for (const auto& c : candidates)
            {
                if (c.idx == m_lockedTargetIndex) continue; // Must be another target

                float toTargetX = c.ndc.x - currentNdc.x;
                float toTargetY = c.ndc.y - currentNdc.y;
                float toTargetLen = std::sqrt(toTargetX * toTargetX + toTargetY * toTargetY);
                if (toTargetLen < 0.001f) continue;

                toTargetX /= toTargetLen;
                toTargetY /= toTargetLen;

                float dot = flickDirX * toTargetX + flickDirY * toTargetY;
                if (dot > bestDot)
                {
                    float score = -dot * 180.0f + c.worldDist * 0.4f + c.screenDist * 40.0f;
                    if (score < bestScore)
                    {
                        bestScore = score;
                        switchIdx = c.idx;
                    }
                }
            }

            if (switchIdx >= 0)
            {
                m_lockedTargetIndex = switchIdx;
                m_prevLockedTargetIndex = switchIdx;
                if (audio) audio->PlayLockOn();
                std::cout << "[TARGET ASSIST] Switched lock-on to target #" << switchIdx
                          << " (Via " << (stickFlick ? "Controller Stick Flick" : "Mouse Flick") << ")" << std::endl;
            }
        }

        // 3. Find best primary target in view
        int bestIdx = -1;
        bool bestIsRemote = false;
        uint8_t bestPlayerId = 0;
        bool bestIsAIBot = false;
        uint8_t bestBotId = 0;
        float bestScore = 999999.0f;
        XMFLOAT2 bestNdc = { 0.0f, 0.0f };
        XMFLOAT3 bestWorldPos = { 0.0f, 0.0f, 0.0f };
        float bestDist = 0.0f;

        for (const auto& c : candidates)
        {
            float maxScreenDist = (m_isHardLockEnabled && m_lockedTargetIndex == c.idx) ? 1.40f : 0.85f;
            if (c.screenDist < maxScreenDist && c.worldDist < 450.0f)
            {
                float lockBonus = (m_isHardLockEnabled && m_lockedTargetIndex == c.idx) ? -800.0f : 0.0f;
                float remoteBonus = c.isRemoteMech ? -500.0f : 0.0f; // Prioritize remote human opponent
                float botBonus = c.isAIBot ? -450.0f : 0.0f;        // Prioritize autonomous combat bot
                float score = c.screenDist * 120.0f + c.worldDist + lockBonus + remoteBonus + botBonus;

                if (score < bestScore)
                {
                    bestScore = score;
                    bestIdx = c.idx;
                    bestIsRemote = c.isRemoteMech;
                    bestPlayerId = c.remotePlayerId;
                    bestIsAIBot = c.isAIBot;
                    bestBotId = c.botId;
                    bestNdc = c.ndc;
                    bestWorldPos = c.worldPos;
                    bestDist = c.worldDist;
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
            m_targetInfo.isRemoteMech = bestIsRemote;
            m_targetInfo.remotePlayerId = bestPlayerId;
            m_targetInfo.isAIBot = bestIsAIBot;
            m_targetInfo.botId = bestBotId;
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
            m_targetInfo.isRemoteMech = false;
            m_targetInfo.remotePlayerId = 0;
            m_targetInfo.isAIBot = false;
            m_targetInfo.botId = 0;
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

    XMFLOAT3 TargetLockSystem::GetAimWorldTarget(const Camera& camera, const XMFLOAT3* overrideLookTarget) const
    {
        if (m_targetInfo.hasTarget)
        {
            return m_targetInfo.worldPos;
        }
        if (overrideLookTarget)
        {
            return *overrideLookTarget;
        }
        return camera.GetLookTarget();
    }
}
