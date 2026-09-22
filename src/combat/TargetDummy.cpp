#include "TargetDummy.hpp"
#include "physics/PhysicsManager.hpp"
#include <iostream>

namespace Overdrive
{
    TargetDummy::TargetDummy(const std::string& name, const XMFLOAT3& position, const XMFLOAT3& size, float maxHp)
        : m_name(name), m_position(position), m_size(size), m_currentHp(maxHp), m_maxHp(maxHp)
    {
    }

    void TargetDummy::InitializePhysics(PhysicsManager* physicsManager)
    {
        if (!physicsManager) return;

        // Create a static bounding box in Jolt Physics
        XMFLOAT3 halfExtent = { m_size.x * 0.5f, m_size.y * 0.5f, m_size.z * 0.5f };
        m_bodyId = physicsManager->CreateStaticBox(m_position, halfExtent);
    }

    void TargetDummy::TakeDamage(float damage)
    {
        if (m_isDestroyed) return;

        m_currentHp -= damage;
        m_hitFlashTimer = 0.15f; // Flash white for 150ms

        std::cout << "[COMBAT] " << m_name << " took " << damage << " damage! Remaining HP: "
                  << std::max(0.0f, m_currentHp) << "/" << m_maxHp << std::endl;

        if (m_currentHp <= 0.0f)
        {
            m_isDestroyed = true;
            m_respawnTimer = 3.5f; // Respawn after 3.5 seconds
            std::cout << "[COMBAT] >> TARGET DESTROYED: " << m_name << " <<" << std::endl;
        }
    }

    void TargetDummy::Update(float deltaTime, PhysicsManager* physicsManager)
    {
        if (m_hitFlashTimer > 0.0f)
        {
            m_hitFlashTimer -= deltaTime;
        }

        if (m_isDestroyed)
        {
            m_respawnTimer -= deltaTime;
            if (m_respawnTimer <= 0.0f)
            {
                // Respawn target
                m_isDestroyed = false;
                m_currentHp = m_maxHp;
                std::cout << "[COMBAT] " << m_name << " RESPAWNED!" << std::endl;
            }
        }
    }
}
