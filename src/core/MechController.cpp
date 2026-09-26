#include "core/MechController.hpp"
#include "physics/PhysicsManager.hpp"
#include "audio/AudioManager.hpp"
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <cmath>
#include <iostream>

namespace Overdrive
{
    MechController::MechController()
    {
        m_position = { 0.0f, 0.0f, 0.0f };
        m_velocity = { 0.0f, 0.0f, 0.0f };
    }

    MechController::~MechController()
    {
        if (m_character)
        {
            m_character->SetListener(nullptr);
        }
        m_character = nullptr;
    }

    void MechController::OnContactSolve(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        JPH::Vec3Arg inContactVelocity,
        const JPH::PhysicsMaterial* inContactMaterial,
        JPH::Vec3Arg inCharacterVelocity,
        JPH::Vec3& ioNewCharacterVelocity)
    {
        // Don't allow the mech to slide down static walkable slopes when not actively moving
        if (!m_allowSliding && inContactVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
        {
            ioNewCharacterVelocity = JPH::Vec3::sZero();
        }
    }

    bool MechController::InitializePhysics(PhysicsManager* physicsManager)
    {
        if (!physicsManager) return false;

        JPH::PhysicsSystem* physicsSystem = physicsManager->GetPhysicsSystem();

        // Capsule colllider: cylinder half-height 0.4m, radius 0.8m (Total height 2.4m)
        // Offset shape so feet are at Y = 0
        JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(0.4f, 0.8f);
        JPH::RotatedTranslatedShapeSettings shapeSettings(JPH::Vec3(0, 1.2f, 0), JPH::Quat::sIdentity(), capsule);
        JPH::RefConst<JPH::Shape> standingShape = shapeSettings.Create().Get();

        JPH::CharacterVirtualSettings settings;
        settings.mShape = standingShape;
        settings.mMaxSlopeAngle = JPH::DegreesToRadians(65.0f); // Easily climb up to 65 degree slopes
        settings.mMaxNumHits = 256;
        settings.mPenetrationRecoverySpeed = 1.0f;
        settings.mPredictiveContactDistance = 0.15f;
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.8f);

        m_character = new JPH::CharacterVirtual(
            &settings,
            JPH::RVec3(m_position.x, m_position.y, m_position.z),
            JPH::Quat::sIdentity(),
            0,
            physicsSystem
        );
        m_character->SetListener(this);

        return true;
    }

    float MechController::GetCurrentSpeed() const
    {
        float speedSq = m_velocity.x * m_velocity.x + m_velocity.y * m_velocity.y + m_velocity.z * m_velocity.z;
        return std::sqrt(speedSq);
    }

    void MechController::TrackTarget(const XMFLOAT3& targetPos, float deltaTime, float speed)
    {
        float dx = targetPos.x - m_position.x;
        float dz = targetPos.z - m_position.z;
        float dy = targetPos.y - (m_position.y + 1.6f);

        float targetDistXZ = std::sqrt(dx * dx + dz * dz);
        if (targetDistXZ < 0.1f) return;

        float desiredYaw = std::atan2(dx, dz);
        if (desiredYaw < 0.0f) desiredYaw += XM_2PI;

        float desiredPitch = -std::atan2(dy, targetDistXZ);
        desiredPitch = std::clamp(desiredPitch, -XM_PIDIV2 * 0.85f, XM_PIDIV2 * 0.85f);

        // Shortest angular difference for yaw
        float diffYaw = desiredYaw - m_yaw;
        while (diffYaw > XM_PI) diffYaw -= XM_2PI;
        while (diffYaw < -XM_PI) diffYaw += XM_2PI;

        float step = std::clamp(speed * deltaTime, 0.0f, 1.0f);
        m_yaw += diffYaw * step;
        if (m_yaw > XM_2PI) m_yaw -= XM_2PI;
        if (m_yaw < 0.0f)   m_yaw += XM_2PI;

        m_pitch += (desiredPitch - m_pitch) * step;
    }

    XMFLOAT3 MechController::GetCockpitHeadPosition() const
    {
        float cosY = std::cos(m_yaw);
        float sinY = std::sin(m_yaw);
        return XMFLOAT3(
            m_position.x + sinY * 0.3f,
            m_position.y + 1.8f,
            m_position.z + cosY * 0.3f
        );
    }

    XMFLOAT3 MechController::GetTPSLookTarget() const
    {
        return XMFLOAT3(m_position.x, m_position.y + 1.6f, m_position.z);
    }

    XMFLOAT3 MechController::GetLeftMuzzlePosition() const
    {
        float abPitch = m_isAssaultBoost ? 0.35f : 0.0f;
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch * 0.3f + abPitch, m_yaw, m_roll);
        // Arm articulation: tilt muzzle up/down around arm socket (y=1.0, z=0.40)
        XMMATRIX armPitchRot = XMMatrixRotationX(-m_aimPitch);
        XMVECTOR socketPos = XMVectorSet(-0.95f, 1.0f, 0.40f - m_recoilLeft, 0.0f);
        XMVECTOR muzzleOffset = XMVectorSet(0.0f, 0.0f, 0.95f, 0.0f);
        XMVECTOR armPos = XMVector3TransformCoord(muzzleOffset, armPitchRot) + socketPos;

        float hoverBob = GetHoverBobOffset();
        XMVECTOR rootPos = XMLoadFloat3(&m_position) + XMVectorSet(0.0f, hoverBob, 0.0f, 0.0f);
        XMVECTOR worldPos = XMVector3TransformCoord(armPos, rot) + rootPos;
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldPos);
        return result;
    }

    XMFLOAT3 MechController::GetRightMuzzlePosition() const
    {
        float abPitch = m_isAssaultBoost ? 0.35f : 0.0f;
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch * 0.3f + abPitch, m_yaw, m_roll);
        // Arm articulation: tilt muzzle up/down around arm socket (y=1.0, z=0.40)
        XMMATRIX armPitchRot = XMMatrixRotationX(-m_aimPitch);
        XMVECTOR socketPos = XMVectorSet(0.95f, 1.0f, 0.40f - m_recoilRight, 0.0f);
        XMVECTOR muzzleOffset = XMVectorSet(0.0f, 0.0f, 0.95f, 0.0f);
        XMVECTOR armPos = XMVector3TransformCoord(muzzleOffset, armPitchRot) + socketPos;

        float hoverBob = GetHoverBobOffset();
        XMVECTOR rootPos = XMLoadFloat3(&m_position) + XMVectorSet(0.0f, hoverBob, 0.0f, 0.0f);
        XMVECTOR worldPos = XMVector3TransformCoord(armPos, rot) + rootPos;
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldPos);
        return result;
    }

    XMFLOAT3 MechController::GetLeftBackMuzzlePosition() const
    {
        float abPitch = m_isAssaultBoost ? 0.35f : 0.0f;
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch * 0.3f + abPitch, m_yaw, m_roll);
        // Left shoulder missile pod top hatch
        XMVECTOR localPos = XMVectorSet(-0.80f, 2.35f, 0.15f, 1.0f);
        float hoverBob = GetHoverBobOffset();
        XMVECTOR rootPos = XMLoadFloat3(&m_position) + XMVectorSet(0.0f, hoverBob, 0.0f, 0.0f);
        XMVECTOR worldPos = XMVector3TransformCoord(localPos, rot) + rootPos;
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldPos);
        return result;
    }

    XMFLOAT3 MechController::GetRightBackMuzzlePosition() const
    {
        float abPitch = m_isAssaultBoost ? 0.35f : 0.0f;
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch * 0.3f + abPitch, m_yaw, m_roll);
        // Right shoulder missile pod top hatch
        XMVECTOR localPos = XMVectorSet(0.80f, 2.35f, 0.15f, 1.0f);
        float hoverBob = GetHoverBobOffset();
        XMVECTOR rootPos = XMLoadFloat3(&m_position) + XMVectorSet(0.0f, hoverBob, 0.0f, 0.0f);
        XMVECTOR worldPos = XMVector3TransformCoord(localPos, rot) + rootPos;
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldPos);
        return result;
    }

    void MechController::Update(float deltaTime, const MechInputState& input, PhysicsManager* physicsManager, AudioManager* audio)
    {
        if (deltaTime <= 0.0f) return;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // 1. Rotation Update (Yaw and Pitch)
        m_yaw += input.yawDelta;
        m_pitch = std::clamp(m_pitch + input.pitchDelta, -XM_PIDIV2 * 0.85f, XM_PIDIV2 * 0.85f);

        if (m_yaw > XM_2PI) m_yaw -= XM_2PI;
        if (m_yaw < 0.0f)   m_yaw += XM_2PI;

        // ACS & Stagger State Management
        if (m_isStaggered)
        {
            m_staggerTimer -= deltaTime;
            m_isAssaultBoost = false;
            m_isBoostKicking = false;
            m_boostKickTimer = 0.0f;
            m_qbTimer = 0.0f;
            m_isAscending = false;

            m_staggerAlarmTimer += deltaTime;
            if (m_staggerAlarmTimer >= 0.40f)
            {
                m_staggerAlarmTimer = 0.0f;
                if (audio)
                {
                    audio->PlayStaggerAlarm();
                }
            }

            if (m_staggerTimer <= 0.0f)
            {
                m_isStaggered = false;
                m_currentAcs = 0.0f;
                m_staggerTimer = 0.0f;
                if (audio)
                {
                    audio->PlaySystemRestored();
                }
                std::cout << "[COMBAT] LOCAL MECH SYSTEM RESTORED from Stagger." << std::endl;
            }
        }
        else if (m_currentAcs > 0.0f)
        {
            if (m_acsCooldownTimer > 0.0f)
            {
                m_acsCooldownTimer -= deltaTime;
            }
            else
            {
                m_currentAcs = std::max(0.0f, m_currentAcs - deltaTime * m_acsRecoveryRate);
            }
        }

        // Create sanitized input (disabled if staggered or destroyed)
        MechInputState effectiveInput = input;
        if (m_isStaggered || m_isDestroyed)
        {
            effectiveInput.moveForward = 0.0f;
            effectiveInput.moveRight = 0.0f;
            effectiveInput.boostToggle = false;
            effectiveInput.quickBoost = false;
            effectiveInput.jumpHold = false;
            effectiveInput.assaultBoost = false;
        }

        // 2. Toggle Boost Mode (Tab / B button)
        if (effectiveInput.boostToggle)
        {
            m_boostOn = !m_boostOn;
        }

        // 3. Assault Boost (AB) Toggle / Boost Kick / Brake Cancel
        if (m_isAssaultBoost && effectiveInput.moveForward < -0.3f)
        {
            // Cancel Assault Boost on backward input (S key or left stick pulled back)
            m_isAssaultBoost = false;
            m_velocity.x *= 0.30f;
            m_velocity.z *= 0.30f;
        }
        else if (effectiveInput.assaultBoost && m_isAssaultBoost && m_boostKickTimer <= 0.0f)
        {
            // Trigger Boost Kick if pressing AB key again during AB
            m_isAssaultBoost = false;
            m_isBoostKicking = true;
            m_boostKickTimer = c_boostKickDuration;
            m_boostKickHit   = false;
            m_energy -= 200.0f;
            if (m_energy < 0.0f) m_energy = 0.0f;
            m_enCooldownTimer = 1.0f;

            if (audio)
            {
                audio->PlayBoostKickThrust(m_position);
            }
        }
        else if (effectiveInput.assaultBoost && !m_isAssaultBoost && m_boostKickTimer <= 0.0f && m_energy > 100.0f)
        {
            m_isAssaultBoost = true;
        }

        if (m_isAssaultBoost && m_energy <= 10.0f)
        {
            m_isAssaultBoost = false;
            m_enCooldownTimer = 1.2f;
        }

        // 4. Quick Boost (QB) Trigger (Shift / X button)
        if (effectiveInput.quickBoost && m_qbTimer <= 0.0f && m_energy >= m_qbEnergyCost)
        {
            m_energy -= m_qbEnergyCost;
            m_qbTimer = c_qbDuration;
            m_enCooldownTimer = m_enCooldownDuration;

            if (audio)
            {
                audio->PlayQuickBoost(m_position);
            }

            float forward = effectiveInput.moveForward;
            float right   = effectiveInput.moveRight;

            if (std::abs(forward) < 0.01f && std::abs(right) < 0.01f)
            {
                forward = 1.0f;
            }

            float len = std::sqrt(forward * forward + right * right);
            forward /= len;
            right   /= len;

            float sinY = std::sin(m_yaw);
            float cosY = std::cos(m_yaw);

            m_qbDirection = XMFLOAT3(
                sinY * forward + cosY * right,
                0.0f,
                cosY * forward - sinY * right
            );

            m_isAssaultBoost = false;
            m_isBoostKicking = false;
            m_boostKickTimer = 0.0f;
            m_roll = -right * 0.25f;

            // AC6 style upward hop impulse on QB: slightly floats up and seamlessly transitions to aerial climb
            m_velocity.y = 5.2f;
            m_isGrounded = false;
        }

        // 5. Jump / Ascend (Space / A button)
        m_isAscending = false;
        if (effectiveInput.jumpHold && m_energy > 20.0f)
        {
            m_isAscending = true;
            if (m_isGrounded)
            {
                m_velocity.y = m_jumpInitial;
                m_isGrounded = false;
                m_energy -= 40.0f;
                m_enCooldownTimer = 0.3f;
            }
            else
            {
                m_velocity.y += c_ascendAccel * deltaTime;
                m_velocity.y = std::min(m_velocity.y, 22.0f);
                m_energy -= 160.0f * deltaTime;
                m_enCooldownTimer = 0.3f;
            }

            if (m_energy <= 5.0f)
            {
                m_enCooldownTimer = 1.0f;
            }
        }

        // 6. Horizontal Movement Calculations
        float sinY = std::sin(m_yaw);
        float cosY = std::cos(m_yaw);

        XMFLOAT3 inputWorldDir = {
            sinY * effectiveInput.moveForward + cosY * effectiveInput.moveRight,
            0.0f,
            cosY * effectiveInput.moveForward - sinY * effectiveInput.moveRight
        };

        float inputLen = std::sqrt(inputWorldDir.x * inputWorldDir.x + inputWorldDir.z * inputWorldDir.z);
        if (inputLen > 1.0f)
        {
            inputWorldDir.x /= inputLen;
            inputWorldDir.z /= inputLen;
            inputLen = 1.0f;
        }

        bool hasMoveInput = (inputLen >= 0.05f);
        bool isSpecialAction = (m_qbTimer > 0.0f) || m_isAssaultBoost || (m_boostKickTimer > 0.0f) || input.jumpHold;

        // Sliding is allowed when in air, actively moving, or boosting/ascending
        m_allowSliding = !m_isGrounded || hasMoveInput || isSpecialAction;

        if (m_boostKickTimer > 0.0f)
        {
            m_boostKickTimer -= deltaTime;
            if (m_boostKickTimer <= 0.0f)
            {
                m_isBoostKicking = false;
            }

            // Extreme rocket dive impulse along Yaw direction (faster than standard AB)
            float kickSpeed = m_abSpeed * 1.35f;
            m_velocity.x = sinY * kickSpeed;
            m_velocity.z = cosY * kickSpeed;
            m_velocity.y = std::sin(-m_pitch) * (kickSpeed * 0.35f);

            m_roll *= 0.8f;
        }
        else if (m_qbTimer > 0.0f)
        {
            float qbRatio = m_qbTimer / c_qbDuration;
            float speed = m_qbSpeed * (0.4f + 0.6f * qbRatio);

            m_velocity.x = m_qbDirection.x * speed;
            m_velocity.z = m_qbDirection.z * speed;

            m_qbTimer -= deltaTime;
        }
        else if (m_isAssaultBoost)
        {
            float abSpeed = m_abSpeed;
            m_velocity.x = sinY * abSpeed;
            m_velocity.z = cosY * abSpeed;
            m_velocity.y = std::sin(-m_pitch) * (abSpeed * 0.4f);

            m_energy -= 200.0f * deltaTime;
            m_enCooldownTimer = 0.5f;
            m_roll *= 0.9f;
        }
        else
        {
            float targetSpeed = m_boostOn ? m_boostSpeed : m_normalSpeed;
            float targetVx = inputWorldDir.x * targetSpeed * inputLen;
            float targetVz = inputWorldDir.z * targetSpeed * inputLen;

            float accelRate = m_isGrounded ? (m_boostOn ? 10.0f : 16.0f) : 5.0f;
            m_velocity.x += (targetVx - m_velocity.x) * accelRate * deltaTime;
            m_velocity.z += (targetVz - m_velocity.z) * accelRate * deltaTime;

            // Inertial banking: bank into turns and lateral strafes
            float turnBank = std::clamp(-input.yawDelta * 3.5f - input.moveRight * 0.12f, -0.22f, 0.22f);
            m_roll += (turnBank - m_roll) * 6.5f * deltaTime;
        }

        // 7. Gravity
        if (!m_isGrounded && !m_isAssaultBoost && (m_boostKickTimer <= 0.0f))
        {
            m_velocity.y -= c_gravity * deltaTime;
        }

        // When grounded on a slope or flat floor and not inputting movement or jumping, lock all velocity to stop sliding
        if (m_isGrounded && !hasMoveInput && !isSpecialAction)
        {
            m_velocity.x = 0.0f;
            m_velocity.y = 0.0f;
            m_velocity.z = 0.0f;
        }

        // 8. Jolt Physics Collision Update
        if (m_character && physicsManager)
        {
            m_character->SetLinearVelocity(JPH::Vec3(m_velocity.x, m_velocity.y, m_velocity.z));

            JPH::PhysicsSystem* physicsSystem = physicsManager->GetPhysicsSystem();
            JPH::TempAllocator* tempAlloc = physicsManager->GetTempAllocator();

            JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
            updateSettings.mStickToFloorStepDown = JPH::Vec3(0, -1.2f, 0); // Tightly stick to slopes
            updateSettings.mWalkStairsStepUp = JPH::Vec3(0, 0.8f, 0);      // Smoothly step over up to 0.8m bumps

            m_character->ExtendedUpdate(
                deltaTime,
                physicsSystem->GetGravity(),
                updateSettings,
                physicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                physicsSystem->GetDefaultLayerFilter(Layers::MOVING),
                {},
                {},
                *tempAlloc
            );

            // Read back resolved position and velocity from Jolt
            JPH::RVec3 newPos = m_character->GetPosition();
            m_position = XMFLOAT3(newPos.GetX(), newPos.GetY(), newPos.GetZ());

            m_isGrounded = (m_character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround);

            JPH::Vec3 newVel = m_character->GetLinearVelocity();
            if (m_isGrounded && !hasMoveInput && !isSpecialAction)
            {
                m_velocity = { 0.0f, 0.0f, 0.0f };
            }
            else
            {
                m_velocity = XMFLOAT3(newVel.GetX(), newVel.GetY(), newVel.GetZ());
            }
        }
        else
        {
            // Fallback ground plane
            m_position.x += m_velocity.x * deltaTime;
            m_position.y += m_velocity.y * deltaTime;
            m_position.z += m_velocity.z * deltaTime;

            if (m_position.y <= 0.0f)
            {
                m_position.y = 0.0f;
                if (m_velocity.y < 0.0f) m_velocity.y = 0.0f;
                m_isGrounded = true;
            }
        }

        // 9. Energy (EN) Recharge System
        if (m_enCooldownTimer > 0.0f)
        {
            m_enCooldownTimer -= deltaTime;
        }
        else
        {
            float rechargeRate = m_isGrounded ? m_enRechargeRate : m_enRechargeRateAir;
            m_energy = std::min(m_maxEnergy, m_energy + rechargeRate * deltaTime);
        }

        // 10. Update Continuous Thruster Audio
        if (audio)
        {
            float curSpeed = GetCurrentSpeed();
            float speedRatio = curSpeed / (m_boostSpeed > 0.0f ? m_boostSpeed : 32.0f);
            bool isThrusterActive = m_boostOn || m_isAssaultBoost || (m_boostKickTimer > 0.0f) || input.jumpHold || (m_qbTimer > 0.0f);
            audio->UpdateBoostSound(isThrusterActive, speedRatio, m_position);
        }

        // 11. Health & Respawn Timers
        if (m_hitFlashTimer > 0.0f) m_hitFlashTimer -= deltaTime;
        if (m_isDestroyed)
        {
            m_respawnTimer -= deltaTime;
            if (m_respawnTimer <= 0.0f)
            {
                Respawn();
            }
        }

        // 12. Procedural Motion & Recoil Updates
        m_recoilRight = std::max(0.0f, m_recoilRight - deltaTime * 4.2f);
        m_recoilLeft  = std::max(0.0f, m_recoilLeft - deltaTime * 4.2f);

        // Suspension shock absorption on touchdown
        if (m_isGrounded && !m_wasGrounded)
        {
            m_landingDip = 0.28f; // Compression dip on landing
        }
        m_wasGrounded = m_isGrounded;
        m_landingDip = std::max(0.0f, m_landingDip - deltaTime * 3.8f);
        m_hoverAnimTime += deltaTime;

        // 13. Update Damage Indicators (Fade out over time)
        for (auto it = m_damageIndicators.begin(); it != m_damageIndicators.end();)
        {
            it->intensity -= deltaTime / (it->duration > 0.0f ? it->duration : 1.0f);
            if (it->intensity <= 0.0f)
            {
                it = m_damageIndicators.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void MechController::TriggerRecoilRight(float strength)
    {
        m_recoilRight = std::min(0.55f, m_recoilRight + strength);
    }

    void MechController::TriggerRecoilLeft(float strength)
    {
        m_recoilLeft = std::min(0.55f, m_recoilLeft + strength);
    }

    float MechController::GetHoverBobOffset() const
    {
        float bob = 0.0f;
        if (m_isGrounded && m_boostOn)
        {
            bob = std::sin(m_hoverAnimTime * 4.8f) * 0.038f;
        }
        return bob - m_landingDip;
    }

    void MechController::TakeDamage(float damage, float impact, float directHitMult, const XMFLOAT3* hitSourceWorldPos, AudioManager* audio)
    {
        if (m_isDestroyed) return;

        float finalDmg = damage;
        if (m_isStaggered)
        {
            finalDmg = damage * directHitMult;
            std::cout << "[COMBAT] >> LOCAL MECH DIRECT HIT TAKEN! (" << static_cast<int>(finalDmg) << " dmg) <<" << std::endl;
        }
        else
        {
            m_currentAcs += impact;
            m_acsCooldownTimer = 2.4f; // 2.4s before ACS cooldown decay begins
            if (m_currentAcs >= m_maxAcs)
            {
                m_isStaggered = true;
                m_currentAcs = m_maxAcs;
                m_staggerTimer = c_staggerDuration;
                m_staggerAlarmTimer = 0.0f; // Immediate alarm trigger
                if (audio)
                {
                    audio->PlayStaggerBreak(m_position);
                    audio->PlayStaggerAlarm();
                }
                std::cout << "[COMBAT] >> !! LOCAL MECH STAGGER OVERLOAD !! <<" << std::endl;
            }
        }

        m_currentHp = std::max(0.0f, m_currentHp - finalDmg);
        m_hitFlashTimer = 0.15f;

        if (hitSourceWorldPos)
        {
            // Vector from local mech to bullet origin / hit source
            float dx = hitSourceWorldPos->x - m_position.x;
            float dz = hitSourceWorldPos->z - m_position.z;
            float len = std::sqrt(dx * dx + dz * dz);
            if (len > 0.1f)
            {
                // World angle of the attacker relative to Z+ (atan2(dx, dz))
                float attackWorldAngle = std::atan2(dx, dz);
                // Relative angle to mech facing yaw (m_yaw: 0 is Z+, PI/2 is X+)
                float relAngle = attackWorldAngle - m_yaw;
                while (relAngle > XM_PI)  relAngle -= XM_2PI;
                while (relAngle < -XM_PI) relAngle += XM_2PI;

                DamageIndicator ind;
                ind.relativeAngle = relAngle;
                ind.intensity = 1.0f;
                ind.duration = 0.95f; // Show hit arc for ~1.0 second
                m_damageIndicators.push_back(ind);
            }
        }

        if (m_currentHp <= 0.0f)
        {
            m_isDestroyed = true;
            m_isStaggered = false;
            m_currentAcs = 0.0f;
            m_respawnTimer = 3.5f;
            std::cout << "[COMBAT] LOCAL MECH DESTROYED! Respawning in 3.5s..." << std::endl;
        }
    }

    void MechController::Respawn(const XMFLOAT3& spawnPos, float spawnYaw)
    {
        m_currentHp = m_maxHp;
        m_isDestroyed = false;
        m_isStaggered = false;
        m_currentAcs = 0.0f;
        m_staggerTimer = 0.0f;
        m_hitFlashTimer = 0.0f;
        m_damageIndicators.clear();
        m_isAssaultBoost = false;
        m_isBoostKicking = false;
        m_boostKickTimer = 0.0f;
        m_boostKickHit   = false;
        m_position = spawnPos;
        m_yaw = spawnYaw;
        m_pitch = 0.0f;
        m_roll = 0.0f;
        m_velocity = { 0.0f, 0.0f, 0.0f };
        if (m_character)
        {
            m_character->SetPosition(JPH::RVec3(spawnPos.x, spawnPos.y, spawnPos.z));
            m_character->SetLinearVelocity(JPH::Vec3::sZero());
        }
        std::cout << "[COMBAT] LOCAL MECH RESPAWNED at (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")!" << std::endl;
    }

    void MechController::ApplyKnockback(const XMFLOAT3& direction, float force)
    {
        float len = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (len > 0.001f)
        {
            m_velocity.x += (direction.x / len) * force;
            m_velocity.y += (direction.y / len) * (force * 0.35f) + 4.5f; // Upward knockback
            m_velocity.z += (direction.z / len) * force;
            m_isGrounded = false;
        }
    }

    void MechController::SetFrameSpecs(float maxHp, float maxEnergy, float normalSpeed, float boostSpeed, float abSpeed, float qbSpeed, float jumpPower, float maxAcs)
    {
        float hpRatio = (m_maxHp > 0.0f) ? (m_currentHp / m_maxHp) : 1.0f;
        m_maxHp = maxHp;
        m_currentHp = m_maxHp * hpRatio;

        float acsRatio = (m_maxAcs > 0.0f) ? (m_currentAcs / m_maxAcs) : 0.0f;
        m_maxAcs = maxAcs;
        m_currentAcs = m_maxAcs * acsRatio;

        float enRatio = (m_maxEnergy > 0.0f) ? (m_energy / m_maxEnergy) : 1.0f;
        m_maxEnergy = maxEnergy;
        m_energy = m_maxEnergy * enRatio;

        m_normalSpeed = normalSpeed;
        m_boostSpeed  = boostSpeed;
        m_abSpeed     = abSpeed;
        m_qbSpeed     = qbSpeed;
        m_jumpInitial = jumpPower;

        std::cout << "[ASSEMBLE] Mech Frame Specs Updated: MaxAP=" << m_maxHp
                  << ", MaxACS=" << m_maxAcs
                  << ", MaxEN=" << m_maxEnergy
                  << ", BoostSpeed=" << m_boostSpeed
                  << ", QBSpeed=" << m_qbSpeed
                  << ", JumpInitial=" << m_jumpInitial << std::endl;
    }

    void MechController::SetInternalSpecs(float qbEnergyCost, float enRechargeRate, float enRechargeRateAir, float enCooldownDuration)
    {
        m_qbEnergyCost       = qbEnergyCost;
        m_enRechargeRate     = enRechargeRate;
        m_enRechargeRateAir  = enRechargeRateAir;
        m_enCooldownDuration = enCooldownDuration;

        std::cout << "[ASSEMBLE] Mech Internal Specs Updated: QBCost=" << m_qbEnergyCost
                  << ", ENRecharge=" << m_enRechargeRate
                  << ", ENDelay=" << m_enCooldownDuration << "s" << std::endl;
    }
}

