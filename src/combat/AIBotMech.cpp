#include "AIBotMech.hpp"
#include "physics/PhysicsManager.hpp"
#include "audio/AudioManager.hpp"
#include "combat/WeaponSystem.hpp"
#include "network/NetworkManager.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Overdrive
{
    AIBotMech::AIBotMech(
        uint8_t botId,
        const std::string& name,
        const XMFLOAT3& spawnPos,
        float spawnYaw,
        const XMFLOAT4& primaryColor,
        const XMFLOAT4& secondaryColor)
        : m_botId(botId)
        , m_botName(name)
        , m_spawnPosition(spawnPos)
        , m_spawnYaw(spawnYaw)
        , m_position(spawnPos)
        , m_yaw(spawnYaw)
        , m_primaryColor(primaryColor)
        , m_secondaryColor(secondaryColor)
    {
    }

    void AIBotMech::InitializePhysics(PhysicsManager* physics)
    {
        if (!physics) return;

        // Bounding box collider for bullet raycast impacts (size 2.2m x 3.6m x 2.2m)
        m_bodyId = physics->CreateStaticBox(
            m_position,
            { 1.1f, 1.8f, 1.1f }
        );
        std::cout << "[AI BOT] Initialized physics collider for " << m_botName << std::endl;
    }

    void AIBotMech::Respawn(const XMFLOAT3& spawnPos, float yaw)
    {
        m_position = spawnPos;
        m_velocity = { 0.0f, 0.0f, 0.0f };
        m_yaw = yaw;
        m_pitch = 0.0f;
        m_roll = 0.0f;
        m_currentHp = m_maxHp;
        m_currentAcs = 0.0f;
        m_isStaggered = false;
        m_staggerTimer = 0.0f;
        m_isDestroyed = false;
        m_respawnTimer = 0.0f;
        m_hitFlashTimer = 0.0f;
        m_boostOn = true;
        m_qbTimer = 0.0f;
        m_isAB = false;
        m_isBoostKick = false;
        m_fireCooldown = 0.5f;
        m_missileCooldown = 2.0f;
    }

    void AIBotMech::TakeDamage(float damage, float impact, float directHitMult, AudioManager* audio)
    {
        if (m_isDestroyed) return;

        float effectiveDamage = damage;
        if (m_isStaggered)
        {
            effectiveDamage *= directHitMult;
        }

        m_currentHp -= effectiveDamage;
        m_hitFlashTimer = 0.12f;

        // ACS Stagger accumulation
        if (!m_isStaggered)
        {
            m_currentAcs += impact;
            m_acsCooldown = 2.2f;

            if (m_currentAcs >= m_maxAcs)
            {
                m_isStaggered = true;
                m_staggerTimer = c_staggerDuration;
                m_currentAcs = m_maxAcs;
                m_velocity = { 0.0f, 0.0f, 0.0f };
                m_isAB = false;
                m_isBoostKick = false;

                if (audio)
                {
                    audio->PlayStaggerBreak(m_position);
                }
                std::cout << "[AI BOT] >> " << m_botName << " ACS OVERLOAD! STAGGERED! <<" << std::endl;
            }
        }

        // On hit, trigger emergency QuickBoost evade if not staggered
        if (!m_isStaggered && m_qbCooldown <= 0.0f && (std::rand() % 100 < 60))
        {
            m_qbTimer = 0.35f;
            m_qbCooldown = 1.8f;
            float strafeYaw = m_yaw + (m_strafeDir > 0.0f ? XM_PIDIV2 : -XM_PIDIV2);
            float qbSpeed = 70.0f;
            m_velocity.x = std::sin(strafeYaw) * qbSpeed;
            m_velocity.z = std::cos(strafeYaw) * qbSpeed;
            if (audio)
            {
                audio->PlayQuickBoost(m_position);
            }
        }

        if (m_currentHp <= 0.0f)
        {
            m_currentHp = 0.0f;
            m_isDestroyed = true;
            m_respawnTimer = 5.0f;
            m_velocity = { 0.0f, 0.0f, 0.0f };

            if (audio)
            {
                audio->PlayExplosion(m_position, 2.0f);
            }
            std::cout << "[AI BOT] >> " << m_botName << " DESTROYED! <<" << std::endl;
        }
    }

    void AIBotMech::ApplyKnockback(const XMFLOAT3& direction, float force)
    {
        if (m_isDestroyed) return;
        m_velocity.x += direction.x * force;
        m_velocity.y += direction.y * force * 0.4f;
        m_velocity.z += direction.z * force;
    }

    void AIBotMech::Update(
        float deltaTime,
        PhysicsManager* physics,
        const XMFLOAT3& playerPos,
        const XMFLOAT3& playerVel,
        bool playerAlive,
        AudioManager* audio,
        WeaponSystem* weaponSys,
        const std::vector<RemoteMech>* remoteMechs)
    {
        if (m_hitFlashTimer > 0.0f)
        {
            m_hitFlashTimer -= deltaTime;
        }

        // 1. Destroyed state & Respawn countdown
        if (m_isDestroyed)
        {
            m_respawnTimer -= deltaTime;
            if (m_respawnTimer <= 0.0f)
            {
                // Choose random respawn waypoint in arena
                static const XMFLOAT3 respawnPoints[4] = {
                    {   0.0f, 2.0f,  35.0f },
                    { -35.0f, 2.0f,   0.0f },
                    {  35.0f, 2.0f,   0.0f },
                    {   0.0f, 2.0f, -35.0f }
                };
                int idx = std::rand() % 4;
                Respawn(respawnPoints[idx], std::rand() % 360 * (XM_PI / 180.0f));
            }
            return;
        }

        // 2. ACS Stagger handling
        if (m_isStaggered)
        {
            m_staggerTimer -= deltaTime;
            m_velocity.x *= std::pow(0.05f, deltaTime);
            m_velocity.z *= std::pow(0.05f, deltaTime);

            if (m_staggerTimer <= 0.0f)
            {
                m_isStaggered = false;
                m_currentAcs = 0.0f;
                // Immediate escape boost
                m_qbTimer = 0.35f;
                m_qbCooldown = 1.5f;
                float retreatYaw = m_yaw + XM_PI;
                m_velocity.x = std::sin(retreatYaw) * 65.0f;
                m_velocity.z = std::cos(retreatYaw) * 65.0f;
            }
            return;
        }

        // Natural ACS Discharge when not taking damage
        if (m_acsCooldown > 0.0f)
        {
            m_acsCooldown -= deltaTime;
        }
        else if (m_currentAcs > 0.0f)
        {
            m_currentAcs = std::max(0.0f, m_currentAcs - 220.0f * deltaTime);
        }

        // 3. Target Selection (Find closest living target between player and remote players)
        XMFLOAT3 targetPos = playerPos;
        XMFLOAT3 targetVel = playerVel;
        bool hasTarget = playerAlive;
        float closestDistSq = 1e9f;

        if (playerAlive)
        {
            float dx = playerPos.x - m_position.x;
            float dy = playerPos.y - m_position.y;
            float dz = playerPos.z - m_position.z;
            closestDistSq = dx * dx + dy * dy + dz * dz;
        }

        if (remoteMechs)
        {
            for (const auto& rMech : *remoteMechs)
            {
                if (rMech.IsAlive())
                {
                    XMFLOAT3 rPos = rMech.GetPosition();
                    float dx = rPos.x - m_position.x;
                    float dy = rPos.y - m_position.y;
                    float dz = rPos.z - m_position.z;
                    float dSq = dx * dx + dy * dy + dz * dz;
                    if (dSq < closestDistSq)
                    {
                        closestDistSq = dSq;
                        targetPos = rPos;
                        targetVel = rMech.GetVelocity();
                        hasTarget = true;
                    }
                }
            }
        }

        if (!hasTarget)
        {
            // No living enemies: slow patrol hover
            m_velocity.x *= 0.92f;
            m_velocity.z *= 0.92f;
            return;
        }

        float toTargetX = targetPos.x - m_position.x;
        float toTargetY = targetPos.y - m_position.y;
        float toTargetZ = targetPos.z - m_position.z;
        float distToTarget = std::sqrt(toTargetX * toTargetX + toTargetY * toTargetY + toTargetZ * toTargetZ);
        if (distToTarget < 0.1f) distToTarget = 0.1f;

        // 4. Lead Prediction & Aim Tracking
        float projectileSpeed = 280.0f;
        float flightTime = distToTarget / projectileSpeed;
        XMFLOAT3 predictedTarget = {
            targetPos.x + targetVel.x * flightTime,
            targetPos.y + targetVel.y * flightTime + 0.8f,
            targetPos.z + targetVel.z * flightTime
        };

        float aimDx = predictedTarget.x - m_position.x;
        float aimDy = predictedTarget.y - m_position.y;
        float aimDz = predictedTarget.z - m_position.z;
        float aimDistHoriz = std::sqrt(aimDx * aimDx + aimDz * aimDz);

        float desiredYaw = std::atan2(aimDx, aimDz);
        float desiredPitch = -std::atan2(aimDy, std::max(0.1f, aimDistHoriz));

        // Smooth servo rotation
        float yawDiff = desiredYaw - m_yaw;
        while (yawDiff > XM_PI)  yawDiff -= XM_2PI;
        while (yawDiff < -XM_PI) yawDiff += XM_2PI;
        m_yaw += yawDiff * std::clamp(deltaTime * 8.5f, 0.0f, 1.0f);
        m_pitch += (desiredPitch - m_pitch) * std::clamp(deltaTime * 8.0f, 0.0f, 1.0f);

        // 5. Tactical Movement AI (State Machine based on distance)
        m_strafeChangeTimer -= deltaTime;
        if (m_strafeChangeTimer <= 0.0f)
        {
            m_strafeDir = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
            m_strafeChangeTimer = 2.0f + (std::rand() % 200) * 0.01f;
        }

        m_qbCooldown -= deltaTime;
        m_verticalThrustTimer -= deltaTime;
        if (m_verticalThrustTimer <= 0.0f && std::rand() % 100 < 5)
        {
            m_verticalThrustTimer = 2.5f;
            m_velocity.y = 18.0f; // Jet ascend jump
        }

        float desiredMoveX = 0.0f;
        float desiredMoveZ = 0.0f;
        float moveSpeed = 38.0f; // Cruise speed

        if (distToTarget > 115.0f)
        {
            // Long Range: Assault Boost charge to close distance
            m_isAB = true;
            m_isBoostKick = false;
            moveSpeed = 82.0f;
            desiredMoveX = std::sin(m_yaw) * moveSpeed;
            desiredMoveZ = std::cos(m_yaw) * moveSpeed;
        }
        else if (distToTarget > 35.0f)
        {
            // Medium Range: Orbital Strafing + Periodic QB Evade
            m_isAB = false;
            m_isBoostKick = false;
            float strafeYaw = m_yaw + (m_strafeDir * XM_PIDIV2);
            float fwdWeight = (distToTarget > 65.0f) ? 0.4f : -0.2f;

            desiredMoveX = (std::sin(strafeYaw) + std::sin(m_yaw) * fwdWeight) * moveSpeed;
            desiredMoveZ = (std::cos(strafeYaw) + std::cos(m_yaw) * fwdWeight) * moveSpeed;

            // Trigger QB Evade
            if (m_qbCooldown <= 0.0f && (std::rand() % 100 < 10))
            {
                m_qbTimer = 0.38f;
                m_qbCooldown = 2.2f;
                float qbSpeed = 74.0f;
                m_velocity.x = std::sin(strafeYaw) * qbSpeed;
                m_velocity.z = std::cos(strafeYaw) * qbSpeed;
                if (audio) audio->PlayQuickBoost(m_position);
            }
        }
        else
        {
            // Close Range (< 35m): Aggressive In-fight or Kick
            moveSpeed = 44.0f;
            if (distToTarget < 18.0f && std::abs(yawDiff) < 0.25f && (std::rand() % 100 < 15))
            {
                // Boost Kick!
                m_isAB = true;
                m_isBoostKick = true;
                m_kickTimer = 0.42f;
                moveSpeed = 92.0f;
                desiredMoveX = std::sin(m_yaw) * moveSpeed;
                desiredMoveZ = std::cos(m_yaw) * moveSpeed;
                if (audio) audio->PlayBoostKickThrust(m_position);
            }
            else
            {
                m_isAB = false;
                m_isBoostKick = false;
                float backYaw = m_yaw + XM_PI + (m_strafeDir * 0.4f);
                desiredMoveX = std::sin(backYaw) * moveSpeed;
                desiredMoveZ = std::cos(backYaw) * moveSpeed;
            }
        }

        // Apply QB impulse decay
        if (m_qbTimer > 0.0f)
        {
            m_qbTimer -= deltaTime;
            m_roll = m_strafeDir * -0.25f;
        }
        else
        {
            m_velocity.x += (desiredMoveX - m_velocity.x) * std::clamp(deltaTime * 6.0f, 0.0f, 1.0f);
            m_velocity.z += (desiredMoveZ - m_velocity.z) * std::clamp(deltaTime * 6.0f, 0.0f, 1.0f);
            m_roll += (0.0f - m_roll) * deltaTime * 5.0f;
        }

        // Gravity & Vertical physics
        m_velocity.y -= 28.0f * deltaTime; // Gravity
        m_position.x += m_velocity.x * deltaTime;
        m_position.y += m_velocity.y * deltaTime;
        m_position.z += m_velocity.z * deltaTime;

        // Ground collision clamp (Floor at Y = 1.0m)
        if (m_position.y < 1.0f)
        {
            m_position.y = 1.0f;
            m_velocity.y = 0.0f;
            m_isGrounded = true;
        }
        else
        {
            m_isGrounded = false;
        }

        // Arena boundary clamp (-120m to +120m)
        m_position.x = std::clamp(m_position.x, -120.0f, 120.0f);
        m_position.z = std::clamp(m_position.z, -120.0f, 120.0f);

        // Update Jolt Physics Body position for raycast hits
        if (physics && !m_bodyId.IsInvalid())
        {
            physics->SetBodyPosition(m_bodyId, m_position);
        }

        // 6. Combat & Weapon Fire Logic
        m_fireCooldown -= deltaTime;
        m_missileCooldown -= deltaTime;

        // Arm weapons fire when roughly on target (< 22 degrees)
        if (std::abs(yawDiff) < 0.38f && weaponSys)
        {
            if (m_fireCooldown <= 0.0f && distToTarget < 140.0f)
            {
                // Alternate between Right Arm and Left Arm
                static bool s_altArm = false;
                s_altArm = !s_altArm;

                XMFLOAT3 muzzle = s_altArm ? GetRightMuzzlePosition() : GetLeftMuzzlePosition();
                WeaponType wType = (m_botId % 2 == 0) ? WeaponType::KineticRifle : WeaponType::BeamRifle;
                float dmg = 95.0f;
                float imp = 65.0f;

                weaponSys->SpawnBotProjectile(muzzle, predictedTarget, wType, dmg, imp, 1.6f, false);
                if (audio)
                {
                    audio->PlayShootRight(muzzle);
                }

                m_fireCooldown = 0.28f + (std::rand() % 15) * 0.01f;
            }

            // Shoulder Missile Salvo
            if (m_missileCooldown <= 0.0f && distToTarget > 25.0f && distToTarget < 160.0f)
            {
                XMFLOAT3 rBackMuzzle = GetRightBackMuzzlePosition();
                weaponSys->SpawnBotProjectile(rBackMuzzle, targetPos, WeaponType::MissilePod, 180.0f, 220.0f, 1.8f, true, targetPos);
                if (audio)
                {
                    audio->PlayShootMissile(rBackMuzzle);
                }
                m_missileCooldown = 4.8f + (std::rand() % 20) * 0.1f;
            }
        }
    }

    XMFLOAT3 AIBotMech::GetRightMuzzlePosition() const
    {
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR offset = XMVectorSet(1.05f, 0.95f, 1.40f, 0.0f);
        XMVECTOR worldOffset = XMVector3Transform(offset, rot);
        XMFLOAT3 res;
        XMStoreFloat3(&res, worldOffset);
        return { m_position.x + res.x, m_position.y + res.y, m_position.z + res.z };
    }

    XMFLOAT3 AIBotMech::GetLeftMuzzlePosition() const
    {
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR offset = XMVectorSet(-1.05f, 0.95f, 1.40f, 0.0f);
        XMVECTOR worldOffset = XMVector3Transform(offset, rot);
        XMFLOAT3 res;
        XMStoreFloat3(&res, worldOffset);
        return { m_position.x + res.x, m_position.y + res.y, m_position.z + res.z };
    }

    XMFLOAT3 AIBotMech::GetRightBackMuzzlePosition() const
    {
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR offset = XMVectorSet(0.85f, 2.30f, 0.40f, 0.0f);
        XMVECTOR worldOffset = XMVector3Transform(offset, rot);
        XMFLOAT3 res;
        XMStoreFloat3(&res, worldOffset);
        return { m_position.x + res.x, m_position.y + res.y, m_position.z + res.z };
    }

    XMFLOAT3 AIBotMech::GetLeftBackMuzzlePosition() const
    {
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
        XMVECTOR offset = XMVectorSet(-0.85f, 2.30f, 0.40f, 0.0f);
        XMVECTOR worldOffset = XMVector3Transform(offset, rot);
        XMFLOAT3 res;
        XMStoreFloat3(&res, worldOffset);
        return { m_position.x + res.x, m_position.y + res.y, m_position.z + res.z };
    }
}
