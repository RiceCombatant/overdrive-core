#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Overdrive
{
    using namespace DirectX;

    class PhysicsManager;
    class AudioManager;
    class WeaponSystem;
    class RemoteMech;

    class AIBotMech
    {
    public:
        AIBotMech(uint8_t botId, const std::string& name, const XMFLOAT3& spawnPos, float spawnYaw,
                  const XMFLOAT4& primaryColor = XMFLOAT4(0.85f, 0.15f, 0.35f, 1.0f),
                  const XMFLOAT4& secondaryColor = XMFLOAT4(0.20f, 0.20f, 0.25f, 1.0f));

        void InitializePhysics(PhysicsManager* physics);
        void Update(
            float deltaTime,
            PhysicsManager* physics,
            const XMFLOAT3& playerPos,
            const XMFLOAT3& playerVel,
            bool playerAlive,
            AudioManager* audio,
            WeaponSystem* weaponSys,
            const std::vector<RemoteMech>* remoteMechs = nullptr
        );

        void TakeDamage(float damage, float impact = 60.0f, float directHitMult = 1.6f, AudioManager* audio = nullptr);
        void ApplyKnockback(const XMFLOAT3& direction, float force);
        void Respawn(const XMFLOAT3& spawnPos, float yaw);

        // Getters
        bool IsActive() const { return m_isActive; }
        void SetActive(bool active) { m_isActive = active; }
        uint8_t GetBotId() const { return m_botId; }
        const std::string& GetName() const { return m_botName; }
        XMFLOAT3 GetPosition() const { return m_position; }
        XMFLOAT3 GetVelocity() const { return m_velocity; }
        XMFLOAT3 GetRotation() const { return XMFLOAT3(m_yaw, m_pitch, m_roll); }
        float GetYaw() const { return m_yaw; }
        float GetPitch() const { return m_pitch; }
        float GetRoll() const { return m_roll; }

        float GetHp() const { return m_currentHp; }
        float GetMaxHp() const { return m_maxHp; }
        float GetHpRatio() const { return m_maxHp > 0.0f ? (m_currentHp / m_maxHp) : 0.0f; }
        float GetAcs() const { return m_currentAcs; }
        float GetMaxAcs() const { return m_maxAcs; }
        float GetAcsRatio() const { return m_maxAcs > 0.0f ? (m_currentAcs / m_maxAcs) : 0.0f; }
        bool IsStaggered() const { return m_isStaggered; }
        float GetStaggerTimer() const { return m_staggerTimer; }
        bool IsAlive() const { return !m_isDestroyed && m_isActive; }
        bool IsDestroyed() const { return m_isDestroyed; }
        bool IsHitFlashing() const { return m_hitFlashTimer > 0.0f; }

        // Boost & Thruster flags for renderer
        bool IsBoostMode() const { return m_boostOn; }
        bool IsQuickBoost() const { return m_qbTimer > 0.0f; }
        bool IsQuickBoosting() const { return m_qbTimer > 0.0f; }
        bool IsAssaultBoost() const { return m_isAB; }
        bool IsAssaultBoosting() const { return m_isAB; }
        bool IsBoostKicking() const { return m_isBoostKick; }

        XMFLOAT4 GetPrimaryColor() const { return m_primaryColor; }
        XMFLOAT4 GetSecondaryColor() const { return m_secondaryColor; }
        JPH::BodyID GetBodyID() const { return m_bodyId; }

        // Sockets for muzzle & weapon rendering
        XMFLOAT3 GetRightMuzzlePosition() const;
        XMFLOAT3 GetLeftMuzzlePosition() const;
        XMFLOAT3 GetRightBackMuzzlePosition() const;
        XMFLOAT3 GetLeftBackMuzzlePosition() const;

    private:
        uint8_t m_botId = 0;
        std::string m_botName = "BOT-01 RAVEN";
        bool m_isActive = true;

        XMFLOAT4 m_primaryColor   = { 0.15f, 0.16f, 0.20f, 1.0f }; // Deep Onyx
        XMFLOAT4 m_secondaryColor = { 0.95f, 0.20f, 0.25f, 1.0f }; // Crimson Red

        XMFLOAT3 m_spawnPosition = { 0.0f, 2.0f, 20.0f };
        float m_spawnYaw = 3.14159f;

        XMFLOAT3 m_position = { 0.0f, 2.0f, 20.0f };
        XMFLOAT3 m_velocity = { 0.0f, 0.0f, 0.0f };
        float m_yaw   = 3.14159f;
        float m_pitch = 0.0f;
        float m_roll  = 0.0f;

        // Thruster & Motion Flags
        bool m_boostOn     = true;
        float m_qbTimer    = 0.0f;
        bool m_isAB        = false;
        bool m_isBoostKick = false;
        float m_kickTimer  = 0.0f;
        bool m_isGrounded  = true;

        // Health & Stagger (ACS)
        float m_currentHp = 3200.0f;
        float m_maxHp     = 3200.0f;
        float m_currentAcs = 0.0f;
        float m_maxAcs     = 1300.0f;
        bool  m_isStaggered = false;
        float m_staggerTimer = 0.0f;
        float m_acsCooldown  = 0.0f;
        const float c_staggerDuration = 2.4f;

        float m_hitFlashTimer = 0.0f;
        bool  m_isDestroyed   = false;
        float m_respawnTimer  = 0.0f;

        // AI Tactics & Behavior timers
        float m_fireCooldown   = 0.0f;
        float m_missileCooldown = 3.0f;
        float m_qbCooldown     = 2.0f;
        float m_decisionTimer  = 0.0f;
        float m_strafeDir      = 1.0f;
        float m_strafeChangeTimer = 2.5f;
        float m_verticalThrustTimer = 0.0f;

        JPH::BodyID m_bodyId;
    };
}
