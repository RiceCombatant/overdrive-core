#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <DirectXMath.h>
#include <algorithm>
#include <memory>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace Overdrive
{
    using namespace DirectX;

    class PhysicsManager;

    enum class MechState
    {
        GroundedNormal,
        GroundedBoost,
        InAir,
        QuickBoost,
        AssaultBoost
    };

    struct MechInputState
    {
        float moveForward = 0.0f; // -1.0 to 1.0
        float moveRight   = 0.0f; // -1.0 to 1.0
        bool boostToggle  = false; // Tab / B button
        bool quickBoost   = false; // Shift / X button
        bool jumpHold     = false; // Space / A button
        bool assaultBoost = false; // Ctrl / L3 button
        float yawDelta    = 0.0f; // Mouse / R stick X
        float pitchDelta  = 0.0f; // Mouse / R stick Y
    };

    class MechController : public JPH::CharacterContactListener
    {
    public:
        MechController();
        virtual ~MechController() override;

        bool InitializePhysics(PhysicsManager* physicsManager);
        void Update(float deltaTime, const MechInputState& input, PhysicsManager* physicsManager);

        // JPH::CharacterContactListener callback to prevent sliding on slopes when stationary
        virtual void OnContactSolve(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::BodyID& inBodyID2,
            const JPH::SubShapeID& inSubShapeID2,
            JPH::RVec3Arg inContactPosition,
            JPH::Vec3Arg inContactNormal,
            JPH::Vec3Arg inContactVelocity,
            const JPH::PhysicsMaterial* inContactMaterial,
            JPH::Vec3Arg inCharacterVelocity,
            JPH::Vec3& ioNewCharacterVelocity) override;

        // Getters
        XMFLOAT3 GetPosition() const { return m_position; }
        XMFLOAT3 GetVelocity() const { return m_velocity; }
        float GetYaw() const { return m_yaw; }
        float GetPitch() const { return m_pitch; }
        float GetRoll() const { return m_roll; }
        float GetCurrentSpeed() const;

        // Smoothly rotate yaw/pitch towards a target position for AC6 Hard-Lock / Target Assist
        void TrackTarget(const XMFLOAT3& targetPos, float deltaTime, float speed = 12.0f);

        float GetEnergy() const { return m_energy; }
        float GetMaxEnergy() const { return m_maxEnergy; }
        float GetEnergyRatio() const { return m_energy / m_maxEnergy; }

        bool IsBoostMode() const { return m_boostOn; }
        bool IsAssaultBoost() const { return m_isAssaultBoost; }
        bool IsGrounded() const { return m_isGrounded; }

        // Camera & Weapon sockets
        XMFLOAT3 GetCockpitHeadPosition() const;
        XMFLOAT3 GetTPSLookTarget() const;
        XMFLOAT3 GetLeftMuzzlePosition() const;
        XMFLOAT3 GetRightMuzzlePosition() const;

    private:
        // Transform
        XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_velocity = { 0.0f, 0.0f, 0.0f };
        float m_yaw   = 0.0f; // in radians
        float m_pitch = 0.0f; // in radians
        float m_roll  = 0.0f; // in radians (banking during QB/turn)

        // States
        bool m_isGrounded     = true;
        bool m_boostOn        = true; // Default to boost mode (hover) as in AC6
        bool m_isAssaultBoost = false;
        bool m_allowSliding   = true; // False when stationary on ground to lock slope sliding
        
        // Quick Boost timer
        float m_qbTimer       = 0.0f;
        XMFLOAT3 m_qbDirection = { 0.0f, 0.0f, 1.0f };

        // Energy (EN)
        float m_energy           = 1000.0f;
        const float m_maxEnergy  = 1000.0f;
        float m_enCooldownTimer  = 0.0f;

        // Constants
        const float c_normalSpeed = 12.0f;
        const float c_boostSpeed  = 28.0f;
        const float c_abSpeed     = 65.0f;
        const float c_qbSpeed     = 70.0f;
        const float c_qbDuration  = 0.32f;
        const float c_gravity     = 28.0f;
        const float c_jumpInitial = 15.0f;
        const float c_ascendAccel = 38.0f;

        // Jolt Physics Character Controller
        JPH::Ref<JPH::CharacterVirtual> m_character;
    };
}
