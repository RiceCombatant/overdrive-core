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

        void FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos);
        void FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos);

        void Update(float deltaTime, PhysicsManager* physicsManager, std::vector<TargetDummy>& targets);

        const std::vector<Projectile>& GetProjectiles() const { return m_projectiles; }

        int GetRightAmmo() const { return m_rightAmmo; }
        int GetLeftAmmo() const { return m_leftAmmo; }
        int GetMaxAmmo() const { return m_maxAmmo; }

    private:
        std::vector<Projectile> m_projectiles;

        float m_rightCooldown = 0.0f;
        float m_leftCooldown  = 0.0f;
        const float c_fireRate = 0.18f; // ~5.5 rounds per second

        int m_rightAmmo = 120;
        int m_leftAmmo  = 120;
        const int m_maxAmmo = 120;

        const float c_projectileSpeed = 280.0f; // High-velocity beam/bullet
    };
}
