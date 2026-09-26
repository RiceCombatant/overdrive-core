#include "TargetDummy.hpp"
#include "physics/PhysicsManager.hpp"
#include "audio/AudioManager.hpp"
#include <iostream>
#include <algorithm>

namespace Overdrive
{
    TargetDummy::TargetDummy(const std::string& name, const XMFLOAT3& position, const XMFLOAT3& size, float maxHp, float maxAcs)
        : m_name(name), m_position(position), m_size(size), m_currentHp(maxHp), m_maxHp(maxHp), m_maxAcs(maxAcs)
    {
    }

    void TargetDummy::InitializePhysics(PhysicsManager* physicsManager)
    {
        if (!physicsManager) return;

        // Create a static bounding box in Jolt Physics
        XMFLOAT3 halfExtent = { m_size.x * 0.5f, m_size.y * 0.5f, m_size.z * 0.5f };
        m_bodyId = physicsManager->CreateStaticBox(m_position, halfExtent);
    }

    void TargetDummy::TakeDamage(float damage, float impact, float directHitMult, AudioManager* audio)
    {
        if (m_isDestroyed) return;

        float finalDmg = damage;
        if (m_isStaggered)
        {
            finalDmg = damage * directHitMult;
            std::cout << "[COMBAT] >> DIRECT HIT on " << m_name << "! (" << static_cast<int>(finalDmg) << " dmg) <<" << std::endl;
        }
        else
        {
            m_currentAcs += impact;
            m_acsCooldown = 2.2f; // 2.2s before ACS cooldown decay begins
            if (m_currentAcs >= m_maxAcs)
            {
                m_isStaggered = true;
                m_currentAcs = m_maxAcs;
                m_staggerTimer = c_staggerDuration;
                if (audio)
                {
                    audio->PlayStaggerBreak(m_position);
                }
                std::cout << "[COMBAT] >> ! TARGET STAGGERED: " << m_name << " ! <<" << std::endl;
            }
        }

        m_currentHp -= finalDmg;
        m_hitFlashTimer = 0.15f; // Flash white for 150ms

        if (m_currentHp <= 0.0f)
        {
            m_isDestroyed = true;
            m_isStaggered = false;
            m_currentAcs = 0.0f;
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

        if (m_isStaggered)
        {
            m_staggerTimer -= deltaTime;
            if (m_staggerTimer <= 0.0f)
            {
                m_isStaggered = false;
                m_currentAcs = 0.0f;
                m_staggerTimer = 0.0f;
                std::cout << "[COMBAT] " << m_name << " recovered from Stagger." << std::endl;
            }
        }
        else if (m_currentAcs > 0.0f)
        {
            if (m_acsCooldown > 0.0f)
            {
                m_acsCooldown -= deltaTime;
            }
            else
            {
                m_currentAcs = std::max(0.0f, m_currentAcs - deltaTime * 320.0f);
            }
        }

        if (m_isDestroyed)
        {
            m_respawnTimer -= deltaTime;
            if (m_respawnTimer <= 0.0f)
            {
                // Respawn target
                m_isDestroyed = false;
                m_isStaggered = false;
                m_currentAcs = 0.0f;
                m_currentHp = m_maxHp;
                std::cout << "[COMBAT] " << m_name << " RESPAWNED!" << std::endl;
            }
        }
    }
}
