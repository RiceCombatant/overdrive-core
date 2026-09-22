#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Overdrive
{
    using namespace DirectX;

    class PhysicsManager;

    class TargetDummy
    {
    public:
        TargetDummy(const std::string& name, const XMFLOAT3& position, const XMFLOAT3& size, float maxHp = 400.0f);

        void InitializePhysics(PhysicsManager* physicsManager);
        void Update(float deltaTime, PhysicsManager* physicsManager);
        void TakeDamage(float damage);

        // Getters
        const std::string& GetName() const { return m_name; }
        XMFLOAT3 GetPosition() const { return m_position; }
        XMFLOAT3 GetSize() const { return m_size; }
        float GetHpRatio() const { return m_currentHp / m_maxHp; }
        bool IsDestroyed() const { return m_isDestroyed; }
        bool IsAlive() const { return !m_isDestroyed; }
        bool IsHitFlashing() const { return m_hitFlashTimer > 0.0f; }
        JPH::BodyID GetBodyID() const { return m_bodyId; }

    private:
        std::string m_name;
        XMFLOAT3 m_position;
        XMFLOAT3 m_size;
        float m_currentHp = 400.0f;
        const float m_maxHp = 400.0f;

        float m_hitFlashTimer = 0.0f;
        bool m_isDestroyed = false;
        float m_respawnTimer = 0.0f;

        JPH::BodyID m_bodyId;
    };
}
