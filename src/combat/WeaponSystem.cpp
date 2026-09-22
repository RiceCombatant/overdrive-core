#include "WeaponSystem.hpp"
#include "physics/PhysicsManager.hpp"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <cmath>
#include <iostream>

namespace Overdrive
{
    WeaponSystem::WeaponSystem()
    {
    }

    void WeaponSystem::FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos)
    {
        if (m_rightCooldown > 0.0f || m_rightAmmo <= 0) return;

        m_rightCooldown = c_fireRate;
        m_rightAmmo--;

        // Calculate direction towards target
        float dx = targetPos.x - muzzlePos.x;
        float dy = targetPos.y - muzzlePos.y;
        float dz = targetPos.z - muzzlePos.z;
        float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.001f) len = 1.0f;

        XMFLOAT3 dir = { dx / len, dy / len, dz / len };

        Projectile p;
        p.position     = muzzlePos;
        p.prevPosition = muzzlePos;
        p.velocity     = { dir.x * c_projectileSpeed, dir.y * c_projectileSpeed, dir.z * c_projectileSpeed };
        p.lifetime     = 2.0f;
        p.damage       = 120.0f;
        p.color        = { 1.0f, 0.55f, 0.15f, 1.0f }; // High-energy Orange/Gold tracer

        m_projectiles.push_back(p);
    }

    void WeaponSystem::FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos)
    {
        if (m_leftCooldown > 0.0f || m_leftAmmo <= 0) return;

        m_leftCooldown = c_fireRate;
        m_leftAmmo--;

        float dx = targetPos.x - muzzlePos.x;
        float dy = targetPos.y - muzzlePos.y;
        float dz = targetPos.z - muzzlePos.z;
        float len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (len < 0.001f) len = 1.0f;

        XMFLOAT3 dir = { dx / len, dy / len, dz / len };

        Projectile p;
        p.position     = muzzlePos;
        p.prevPosition = muzzlePos;
        p.velocity     = { dir.x * c_projectileSpeed, dir.y * c_projectileSpeed, dir.z * c_projectileSpeed };
        p.lifetime     = 2.0f;
        p.damage       = 120.0f;
        p.color        = { 0.25f, 0.85f, 1.0f, 1.0f }; // Electric Cyan beam

        m_projectiles.push_back(p);
    }

    void WeaponSystem::Update(float deltaTime, PhysicsManager* physicsManager, std::vector<TargetDummy>& targets)
    {
        if (m_rightCooldown > 0.0f) m_rightCooldown -= deltaTime;
        if (m_leftCooldown  > 0.0f) m_leftCooldown  -= deltaTime;

        JPH::PhysicsSystem* physicsSystem = physicsManager ? physicsManager->GetPhysicsSystem() : nullptr;

        for (auto it = m_projectiles.begin(); it != m_projectiles.end();)
        {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0.0f)
            {
                it = m_projectiles.erase(it);
                continue;
            }

            it->prevPosition = it->position;
            it->position.x += it->velocity.x * deltaTime;
            it->position.y += it->velocity.y * deltaTime;
            it->position.z += it->velocity.z * deltaTime;

            bool hasHit = false;

            // Jolt Physics Raycast between prevPosition and current position
            if (physicsSystem)
            {
                JPH::RRayCast ray(
                    JPH::RVec3(it->prevPosition.x, it->prevPosition.y, it->prevPosition.z),
                    JPH::Vec3(it->position.x - it->prevPosition.x, it->position.y - it->prevPosition.y, it->position.z - it->prevPosition.z)
                );

                JPH::RayCastResult hitResult;
                if (physicsSystem->GetNarrowPhaseQuery().CastRay(ray, hitResult))
                {
                    hasHit = true;

                    // Check if hit one of our target dummies
                    for (auto& target : targets)
                    {
                        if (!target.IsDestroyed() && target.GetBodyID() == hitResult.mBodyID)
                        {
                            target.TakeDamage(it->damage);
                            break;
                        }
                    }
                }
            }

            if (hasHit)
            {
                it = m_projectiles.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
