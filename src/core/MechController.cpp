#include "MechController.hpp"
#include "physics/PhysicsManager.hpp"
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <cmath>

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
        XMVECTOR localPos = XMVectorSet(-0.95f, 1.0f, 1.35f, 1.0f);
        XMVECTOR worldPos = XMVector3TransformCoord(localPos, rot) + XMLoadFloat3(&m_position);
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldPos);
        return result;
    }

    XMFLOAT3 MechController::GetRightMuzzlePosition() const
    {
        float abPitch = m_isAssaultBoost ? 0.35f : 0.0f;
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch * 0.3f + abPitch, m_yaw, m_roll);
        XMVECTOR localPos = XMVectorSet(0.95f, 1.0f, 1.35f, 1.0f);
        XMVECTOR worldPos = XMVector3TransformCoord(localPos, rot) + XMLoadFloat3(&m_position);
        XMFLOAT3 result;
        XMStoreFloat3(&result, worldPos);
        return result;
    }

    void MechController::Update(float deltaTime, const MechInputState& input, PhysicsManager* physicsManager)
    {
        if (deltaTime <= 0.0f) return;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // 1. Rotation Update (Yaw and Pitch)
        m_yaw += input.yawDelta;
        m_pitch = std::clamp(m_pitch + input.pitchDelta, -XM_PIDIV2 * 0.85f, XM_PIDIV2 * 0.85f);

        if (m_yaw > XM_2PI) m_yaw -= XM_2PI;
        if (m_yaw < 0.0f)   m_yaw += XM_2PI;

        // 2. Toggle Boost Mode (Tab / B button)
        if (input.boostToggle)
        {
            m_boostOn = !m_boostOn;
        }

        // 3. Assault Boost (AB) Toggle / State
        if (input.assaultBoost && !m_isAssaultBoost && m_energy > 100.0f)
        {
            m_isAssaultBoost = true;
        }
        else if (input.assaultBoost && m_isAssaultBoost)
        {
            m_isAssaultBoost = false;
        }

        if (m_isAssaultBoost && m_energy <= 10.0f)
        {
            m_isAssaultBoost = false;
            m_enCooldownTimer = 1.2f;
        }

        // 4. Quick Boost (QB) Trigger (Shift / X button)
        if (input.quickBoost && m_qbTimer <= 0.0f && m_energy >= 220.0f)
        {
            m_energy -= 220.0f;
            m_qbTimer = c_qbDuration;
            m_enCooldownTimer = 0.45f;

            float forward = input.moveForward;
            float right   = input.moveRight;

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
            m_roll = -right * 0.25f;

            // AC6 style upward hop impulse on QB: slightly floats up and seamlessly transitions to aerial climb
            m_velocity.y = 5.2f;
            m_isGrounded = false;
        }

        // 5. Jump / Ascend (Space / A button)
        if (input.jumpHold && m_energy > 20.0f)
        {
            if (m_isGrounded)
            {
                m_velocity.y = c_jumpInitial;
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
            sinY * input.moveForward + cosY * input.moveRight,
            0.0f,
            cosY * input.moveForward - sinY * input.moveRight
        };

        float inputLen = std::sqrt(inputWorldDir.x * inputWorldDir.x + inputWorldDir.z * inputWorldDir.z);
        if (inputLen > 1.0f)
        {
            inputWorldDir.x /= inputLen;
            inputWorldDir.z /= inputLen;
            inputLen = 1.0f;
        }

        bool hasMoveInput = (inputLen >= 0.05f);
        bool isSpecialAction = (m_qbTimer > 0.0f) || m_isAssaultBoost || input.jumpHold;

        // Sliding is allowed when in air, actively moving, or boosting/ascending
        m_allowSliding = !m_isGrounded || hasMoveInput || isSpecialAction;

        if (m_qbTimer > 0.0f)
        {
            float qbRatio = m_qbTimer / c_qbDuration;
            float speed = c_qbSpeed * (0.4f + 0.6f * qbRatio);

            m_velocity.x = m_qbDirection.x * speed;
            m_velocity.z = m_qbDirection.z * speed;

            m_qbTimer -= deltaTime;
        }
        else if (m_isAssaultBoost)
        {
            float abSpeed = c_abSpeed;
            m_velocity.x = sinY * abSpeed;
            m_velocity.z = cosY * abSpeed;
            m_velocity.y = std::sin(-m_pitch) * (abSpeed * 0.4f);

            m_energy -= 200.0f * deltaTime;
            m_enCooldownTimer = 0.5f;
            m_roll *= 0.9f;
        }
        else
        {
            float targetSpeed = m_boostOn ? c_boostSpeed : c_normalSpeed;
            float targetVx = inputWorldDir.x * targetSpeed * inputLen;
            float targetVz = inputWorldDir.z * targetSpeed * inputLen;

            float accelRate = m_isGrounded ? (m_boostOn ? 10.0f : 16.0f) : 5.0f;
            m_velocity.x += (targetVx - m_velocity.x) * accelRate * deltaTime;
            m_velocity.z += (targetVz - m_velocity.z) * accelRate * deltaTime;

            m_roll += (0.0f - m_roll) * 6.0f * deltaTime;
        }

        // 7. Gravity
        if (!m_isGrounded && !m_isAssaultBoost)
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
            float rechargeRate = m_isGrounded ? 450.0f : 120.0f;
            m_energy = std::min(m_maxEnergy, m_energy + rechargeRate * deltaTime);
        }
    }
}
