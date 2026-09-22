#include "WeaponSystem.hpp"
#include "physics/PhysicsManager.hpp"
#include "audio/AudioManager.hpp"
#include "network/NetworkManager.hpp"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <cmath>
#include <iostream>

namespace Overdrive
{
    WeaponSystem::WeaponSystem()
    {
    }

    bool WeaponSystem::FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio)
    {
        if (m_rightReloading || m_rightAmmo <= 0 || m_rightCooldown > 0.0f) return false;

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

        // Auto-reload when magazine is emptied
        if (m_rightAmmo <= 0)
        {
            ReloadRight(audio);
        }

        return true;
    }

    bool WeaponSystem::FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio)
    {
        if (m_leftReloading || m_leftAmmo <= 0 || m_leftCooldown > 0.0f) return false;

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

        // Auto-reload when magazine is emptied
        if (m_leftAmmo <= 0)
        {
            ReloadLeft(audio);
        }

        return true;
    }

    void WeaponSystem::ReloadRight(AudioManager* audio)
    {
        if (m_rightReloading || m_rightAmmo >= c_maxAmmo) return;
        m_rightReloading = true;
        m_rightReloadTimer = c_reloadDuration;
    }

    void WeaponSystem::ReloadLeft(AudioManager* audio)
    {
        if (m_leftReloading || m_leftAmmo >= c_maxAmmo) return;
        m_leftReloading = true;
        m_leftReloadTimer = c_reloadDuration;
    }

    void WeaponSystem::ReloadBoth(AudioManager* audio)
    {
        ReloadRight(audio);
        ReloadLeft(audio);
    }

    float WeaponSystem::GetRightAmmoRatio() const
    {
        if (m_rightReloading)
        {
            return std::clamp(1.0f - (m_rightReloadTimer / c_reloadDuration), 0.0f, 1.0f);
        }
        return static_cast<float>(m_rightAmmo) / static_cast<float>(c_maxAmmo);
    }

    float WeaponSystem::GetLeftAmmoRatio() const
    {
        if (m_leftReloading)
        {
            return std::clamp(1.0f - (m_leftReloadTimer / c_reloadDuration), 0.0f, 1.0f);
        }
        return static_cast<float>(m_leftAmmo) / static_cast<float>(c_maxAmmo);
    }

    void WeaponSystem::SpawnRemoteProjectile(bool isLeftArm, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos)
    {
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
        p.color        = isLeftArm ? XMFLOAT4(1.0f, 0.15f, 0.25f, 1.0f) : XMFLOAT4(1.0f, 0.45f, 0.1f, 1.0f); // Enemy Crimson / Orange beam

        m_projectiles.push_back(p);
    }

    void WeaponSystem::Update(
        float deltaTime,
        PhysicsManager* physicsManager,
        std::vector<TargetDummy>& targets,
        AudioManager* audio,
        std::vector<RemoteMech>* remoteMechs,
        NetworkManager* network)
    {
        if (m_rightCooldown > 0.0f) m_rightCooldown -= deltaTime;
        if (m_leftCooldown  > 0.0f) m_leftCooldown  -= deltaTime;

        // Update reload timers and restore magazines
        if (m_rightReloading)
        {
            m_rightReloadTimer -= deltaTime;
            if (m_rightReloadTimer <= 0.0f)
            {
                m_rightReloadTimer = 0.0f;
                m_rightReloading = false;
                m_rightAmmo = c_maxAmmo;
                if (audio)
                {
                    audio->PlayReload({ 0.6f, 1.2f, 0.5f });
                }
            }
        }

        if (m_leftReloading)
        {
            m_leftReloadTimer -= deltaTime;
            if (m_leftReloadTimer <= 0.0f)
            {
                m_leftReloadTimer = 0.0f;
                m_leftReloading = false;
                m_leftAmmo = c_maxAmmo;
                if (audio)
                {
                    audio->PlayReload({ -0.6f, 1.2f, 0.5f });
                }
            }
        }

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

            // 1. Proximity check against RemoteMechs (prevents tunneling at 280 m/s)
            if (remoteMechs)
            {
                for (auto& rMech : *remoteMechs)
                {
                    if (!rMech.IsAlive()) continue;

                    XMFLOAT3 rPos = rMech.GetPosition();
                    // Target center is approximately Y + 1.0f
                    float dx = it->position.x - rPos.x;
                    float dy = it->position.y - (rPos.y + 1.0f);
                    float dz = it->position.z - rPos.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq <= 2.2f * 2.2f) // Hit radius 2.2m
                    {
                        hasHit = true;
                        rMech.TakeDamage(it->damage);
                        if (network)
                        {
                            network->SendHitEvent(rMech.GetPlayerId(), it->damage, it->position);
                        }
                        if (audio)
                        {
                            audio->PlayExplosion(it->position, 1.25f);
                        }
                        break;
                    }
                }
            }

            // 2. Jolt Physics Raycast between prevPosition and current position
            if (!hasHit && physicsSystem)
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
                    bool hitRemote = false;

                    // Check if hit remote mech via Jolt BodyID
                    if (remoteMechs)
                    {
                        for (auto& rMech : *remoteMechs)
                        {
                            if (rMech.IsAlive() && !rMech.GetBodyID().IsInvalid() && hitResult.mBodyID == rMech.GetBodyID())
                            {
                                hitRemote = true;
                                rMech.TakeDamage(it->damage);
                                if (network)
                                {
                                    network->SendHitEvent(rMech.GetPlayerId(), it->damage, it->position);
                                }
                                if (audio)
                                {
                                    audio->PlayExplosion(it->position, 1.25f);
                                }
                                break;
                            }
                        }
                    }

                    if (!hitRemote)
                    {
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
