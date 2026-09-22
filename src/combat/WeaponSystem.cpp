#include "WeaponSystem.hpp"
#include "physics/PhysicsManager.hpp"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include "combat/WeaponSystem.hpp"
#include "audio/AudioManager.hpp"
#include <cmath>
#include <iostream>

namespace Overdrive
{
    WeaponSystem::WeaponSystem()
    {
    }

    void WeaponSystem::FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio)
    {
        if (m_rightAmmo <= 0 || m_rightCooldown > 0.0f) return;

        m_rightCooldown = c_fireRate;
        m_rightAmmo--;

        if (audio)
        {
            audio->PlayShootRight(muzzlePos);
        }

        XMFLOAT3 dir = {
            targetPos.x - muzzlePos.x,
            targetPos.y - muzzlePos.y,
            targetPos.z - muzzlePos.z
        };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.001f)
        {
            dir.x /= len;
            dir.y /= len;
            dir.z /= len;
        }

        Projectile p;
        p.position     = muzzlePos;
        p.prevPosition = muzzlePos;
        p.velocity     = { dir.x * c_projectileSpeed, dir.y * c_projectileSpeed, dir.z * c_projectileSpeed };
        p.lifetime     = 2.0f;
        p.damage       = 120.0f;
        p.color        = { 1.0f, 0.55f, 0.15f, 1.0f }; // Kinetic Orange tracer

        m_projectiles.push_back(p);
    }

    void WeaponSystem::FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio)
    {
        if (m_leftAmmo <= 0 || m_leftCooldown > 0.0f) return;

        m_leftCooldown = c_fireRate;
        m_leftAmmo--;

        if (audio)
        {
            audio->PlayShootLeft(muzzlePos);
        }

        XMFLOAT3 dir = {
            targetPos.x - muzzlePos.x,
            targetPos.y - muzzlePos.y,
            targetPos.z - muzzlePos.z
        };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.001f)
        {
            dir.x /= len;
            dir.y /= len;
            dir.z /= len;
        }

        Projectile p;
        p.position     = muzzlePos;
        p.prevPosition = muzzlePos;
        p.velocity     = { dir.x * c_projectileSpeed, dir.y * c_projectileSpeed, dir.z * c_projectileSpeed };
        p.lifetime     = 2.0f;
        p.damage       = 120.0f;
        p.color        = { 0.25f, 0.85f, 1.0f, 1.0f }; // Electric Cyan beam

        m_projectiles.push_back(p);
    }

    void WeaponSystem::Update(float deltaTime, PhysicsManager* physicsManager, std::vector<TargetDummy>& targets, AudioManager* audio)
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
                    bool targetDestroyed = false;

                    // Check if hit one of our target dummies
                    for (auto& target : targets)
                    {
                        if (!target.IsDestroyed() && target.GetBodyID() == hitResult.mBodyID)
                        {
                            target.TakeDamage(it->damage);
                            if (target.IsDestroyed())
                            {
                                targetDestroyed = true;
                            }
                            break;
                        }
                    }

                    if (audio)
                    {
                        audio->PlayExplosion(it->position, targetDestroyed ? 1.25f : 0.85f);
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
