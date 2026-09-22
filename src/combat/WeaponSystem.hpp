#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include "combat/TargetDummy.hpp"
#include "math/MathTypes.hpp"

namespace Overdrive
{
    using namespace DirectX;

    class PhysicsManager;
    class AudioManager;
    class RemoteMech;
    class NetworkManager;

    struct Projectile
    {
        XMFLOAT3 position;
        XMFLOAT3 prevPosition;
        XMFLOAT3 velocity;
        float lifetime = 2.0f;
        XMFLOAT4 color;
        float damage = 120.0f;
    };

    class WeaponSystem
    {
    public:
        WeaponSystem();

        // Returns true if projectile was actually fired (for network sync)
        bool FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr);
        bool FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr);
        void SpawnRemoteProjectile(bool isLeftArm, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos);

        // Manual reload triggers
        void ReloadRight(AudioManager* audio = nullptr);
        void ReloadLeft(AudioManager* audio = nullptr);
        void ReloadBoth(AudioManager* audio = nullptr);

        void Update(
            float deltaTime,
            PhysicsManager* physicsManager,
            std::vector<TargetDummy>& targets,
            AudioManager* audio = nullptr,
            std::vector<RemoteMech>* remoteMechs = nullptr,
            NetworkManager* network = nullptr);

        const std::vector<Projectile>& GetProjectiles() const { return m_projectiles; }

        int GetRightAmmo() const { return m_rightAmmo; }
        int GetLeftAmmo() const { return m_leftAmmo; }
        int GetMaxAmmo() const { return c_maxAmmo; }

        bool IsRightReloading() const { return m_rightReloading; }
        bool IsLeftReloading() const { return m_leftReloading; }

        // Returns 0.0f..1.0f (ammo ratio when ready, or reload charge progress when reloading)
        float GetRightAmmoRatio() const;
        float GetLeftAmmoRatio() const;

    private:
        std::vector<Projectile> m_projectiles;

        float m_rightCooldown = 0.0f;
        float m_leftCooldown  = 0.0f;
        const float c_fireRate = 0.18f; // ~5.5 rounds per second

        const int c_maxAmmo = 30; // 30-round magazine capacity
        int m_rightAmmo = 30;
        int m_leftAmmo  = 30;

        const float c_reloadDuration = 2.8f; // Deliberate sci-fi reload duration
        float m_rightReloadTimer = 0.0f;
        float m_leftReloadTimer  = 0.0f;
        bool m_rightReloading    = false;
        bool m_leftReloading     = false;

        const float c_projectileSpeed = 280.0f; // High-velocity beam/bullet
    };
}
