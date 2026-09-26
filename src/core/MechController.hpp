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
    class AudioManager;

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
        float lookStickX  = 0.0f; // Raw Right Stick X (-1.0 to 1.0)
        float lookStickY  = 0.0f; // Raw Right Stick Y (-1.0 to 1.0)
    };

    class MechController : public JPH::CharacterContactListener
    {
    public:
        MechController();
        virtual ~MechController() override;

        bool InitializePhysics(PhysicsManager* physicsManager);
        void Update(float deltaTime, const MechInputState& input, PhysicsManager* physicsManager, AudioManager* audio = nullptr);

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
        bool IsQuickBoost() const { return m_qbTimer > 0.0f; }
        bool IsAssaultBoost() const { return m_isAssaultBoost; }
        bool IsBoostKicking() const { return m_boostKickTimer > 0.0f; }
        float GetBoostKickTimer() const { return m_boostKickTimer; }
        float GetBoostKickDuration() const { return c_boostKickDuration; }
        bool HasBoostKickHit() const { return m_boostKickHit; }
        void SetBoostKickHit(bool hit) { m_boostKickHit = hit; }
        bool IsAscending() const { return m_isAscending; }
        float GetQbTimer() const { return m_qbTimer; }
        float GetQbDuration() const { return c_qbDuration; }
        bool IsGrounded() const { return m_isGrounded; }

        float GetHp() const { return m_currentHp; }
        float GetMaxHp() const { return m_maxHp; }
        float GetHpRatio() const { return m_currentHp / m_maxHp; }
        float GetAcs() const { return m_currentAcs; }
        float GetMaxAcs() const { return m_maxAcs; }
        float GetAcsRatio() const { return m_maxAcs > 0.0f ? (m_currentAcs / m_maxAcs) : 0.0f; }
        bool IsStaggered() const { return m_isStaggered; }
        float GetStaggerTimer() const { return m_staggerTimer; }
        bool IsDestroyed() const { return m_isDestroyed; }
        bool IsHitFlashing() const { return m_hitFlashTimer > 0.0f; }
        float GetRespawnTimer() const { return m_respawnTimer; }
        struct DamageIndicator
        {
            float relativeAngle; // in radians: 0 = straight ahead, PI = directly behind, +PI/2 = right, -PI/2 = left
            float intensity;     // 1.0 -> 0.0 fade out
            float duration;      // total duration in seconds
        };

        const std::vector<DamageIndicator>& GetDamageIndicators() const { return m_damageIndicators; }
        void TakeDamage(float damage, float impact = 60.0f, float directHitMult = 1.6f, const XMFLOAT3* hitSourceWorldPos = nullptr, AudioManager* audio = nullptr);
        void ApplyKnockback(const XMFLOAT3& direction, float force);
        void Respawn(const XMFLOAT3& spawnPos = { 0.0f, 0.5f, 0.0f }, float spawnYaw = 0.0f);

        // Procedural Motion, Recoil & Aim Sockets
        void TriggerRecoilRight(float strength = 0.22f);
        void TriggerRecoilLeft(float strength = 0.22f);
        float GetRecoilRight() const { return m_recoilRight; }
        float GetRecoilLeft() const { return m_recoilLeft; }

        void SetAimPitch(float pitch) { m_aimPitch = pitch; }
        float GetAimPitch() const { return m_aimPitch; }

        float GetHoverBobOffset() const;
        float GetLandingDip() const { return m_landingDip; }

        // Configure Frame Assembly Specs
        void SetFrameSpecs(float maxHp, float maxEnergy, float normalSpeed, float boostSpeed, float abSpeed, float qbSpeed, float jumpPower, float maxAcs = 1000.0f);

        // Configure Internal Assembly Specs (Booster & Generator)
        void SetInternalSpecs(float qbEnergyCost, float enRechargeRate, float enRechargeRateAir, float enCooldownDuration);

        // Camera & Weapon sockets
        XMFLOAT3 GetCockpitHeadPosition() const;
        XMFLOAT3 GetTPSLookTarget() const;
        XMFLOAT3 GetLeftMuzzlePosition() const;
        XMFLOAT3 GetRightMuzzlePosition() const;
        XMFLOAT3 GetLeftBackMuzzlePosition() const;
        XMFLOAT3 GetRightBackMuzzlePosition() const;

    private:
        // Transform
        XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_velocity = { 0.0f, 0.0f, 0.0f };
        float m_yaw   = 0.0f; // in radians
        float m_pitch = 0.0f; // in radians
        float m_roll  = 0.0f; // in radians (banking during QB/turn)

        // Procedural motion states
        float m_recoilRight   = 0.0f; // Right arm kickback offset (meters)
        float m_recoilLeft    = 0.0f; // Left arm kickback offset (meters)
        float m_aimPitch      = 0.0f; // Weapon pitch aiming elevation angle (radians)
        float m_landingDip    = 0.0f; // Suspension shock compression (meters)
        float m_hoverAnimTime = 0.0f;
        bool  m_wasGrounded   = true;

        // States
        bool m_isGrounded     = true;
        bool m_boostOn        = true; // Default to boost mode (hover) as in AC6
        bool m_isAssaultBoost = false;
        bool m_isBoostKicking = false;
        float m_boostKickTimer = 0.0f;
        bool m_boostKickHit   = false;
        const float c_boostKickDuration = 0.42f;
        bool m_isAscending    = false;
        bool m_allowSliding   = true; // False when stationary on ground to lock slope sliding
        
        // Quick Boost timer
        float m_qbTimer       = 0.0f;
        XMFLOAT3 m_qbDirection = { 0.0f, 0.0f, 1.0f };

        // Energy (EN)
        float m_energy           = 1000.0f;
        float m_maxEnergy        = 1000.0f;
        float m_enCooldownTimer  = 0.0f;
        float m_qbEnergyCost     = 210.0f;
        float m_enRechargeRate   = 480.0f;
        float m_enRechargeRateAir = 130.0f;
        float m_enCooldownDuration = 0.40f;

        // Health / AP (Armor Points)
        float m_currentHp        = 2500.0f;
        float m_maxHp            = 2500.0f;
        bool m_isDestroyed       = false;
        float m_hitFlashTimer    = 0.0f;
        float m_respawnTimer     = 0.0f;
        std::vector<DamageIndicator> m_damageIndicators;

        // ACS (Attitude Control System) & Stagger Attributes
        float m_currentAcs       = 0.0f;
        float m_maxAcs           = 1000.0f;
        bool  m_isStaggered      = false;
        float m_staggerTimer     = 0.0f;
        float m_acsCooldownTimer = 0.0f;
        float m_acsRecoveryRate  = 350.0f;
        const float c_staggerDuration = 2.4f;
        float m_staggerAlarmTimer = 0.0f;

        // Frame Performance Attributes
        float m_normalSpeed = 12.0f;
        float m_boostSpeed  = 28.0f;
        float m_abSpeed     = 65.0f;
        float m_qbSpeed     = 70.0f;
        const float c_qbDuration  = 0.32f;
        const float c_gravity     = 28.0f;
        float m_jumpInitial = 15.0f;
        const float c_ascendAccel = 38.0f;

        // Jolt Physics Character Controller
        JPH::Ref<JPH::CharacterVirtual> m_character;
    };
}
