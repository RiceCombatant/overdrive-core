#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <algorithm>
#include "combat/TargetDummy.hpp"
#include "math/MathTypes.hpp"

namespace Overdrive
{
    using namespace DirectX;

    class PhysicsManager;
    class AudioManager;
    class RemoteMech;
    class NetworkManager;
    class MechController;
    class AIBotMech;

    enum class WeaponSlot : int
    {
        RightArm  = 0,
        LeftArm   = 1,
        RightBack = 2,
        LeftBack  = 3,
        Count     = 4
    };

    enum class WeaponCategory
    {
        ArmWeapon,       // Handheld weapon (can be held in arm or mounted in back hanger rack)
        BackOnlyWeapon   // Dedicated back weapon (e.g. missile pods, heavy back cannons)
    };

    enum class WeaponType
    {
        KineticRifle,     // Balanced assault rifle (kinetic orange tracer)
        BeamRifle,        // High-cycle laser rifle (cyan energy beam)
        HeavyPlasma,      // Heavy hitting plasma rifle (violet plasma bolt, high damage)
        BurstGun,         // Rapid burst SMG (amber kinetic tracer)
        MissilePod,       // 4-cell vertical/homing micro-missile launcher (ML-080)
        HeavyBazooka,     // Heavy explosive cannon (BZ-033 TITAN)
        LaserBlade,       // High-frequency energy blade for melee slash (LB-077 MOONLIGHT)
        GatlingGun,       // 6-barrel rotary machine gun (GT-090 VULCAN)
        SpreadShotgun,    // 8-pellet spread kinetic shotgun (SG-020 BREAKER)
        None
    };

    struct WeaponData
    {
        WeaponType type = WeaponType::None;
        std::string id;
        std::string name;
        std::string shortName;
        WeaponCategory category = WeaponCategory::ArmWeapon;
        int maxAmmo = 30;
        float fireRate = 0.18f;
        float reloadDuration = 2.8f;
        float damage = 120.0f;
        float projectileSpeed = 280.0f;
        XMFLOAT4 projectileColor = { 1.0f, 0.6f, 0.2f, 1.0f };
        // Dimensions for 3D model box visualization (sx, sy, sz)
        XMFLOAT3 modelSize = { 0.20f, 0.25f, 1.20f };
        XMFLOAT4 modelColor = { 0.92f, 0.94f, 0.96f, 1.0f };

        // Extended combat characteristics
        bool isMelee = false;
        int pelletCount = 1;
        float spreadAngle = 0.0f;
        bool isHoming = false;
        float splashRadius = 0.0f;
        float recoilKick = 0.16f; // Procedural weapon kickback distance (meters)
        float impact = 75.0f;     // ACS attitude strain / impact power
        float directHitMult = 1.65f; // Direct Hit damage multiplier when target is staggered
    };

    struct WeaponInstance
    {
        WeaponData data;
        int currentAmmo = 30;
        float cooldown = 0.0f;
        float reloadTimer = 0.0f;
        bool isReloading = false;
        float spinAngle = 0.0f;   // Rotary barrel rotation angle (radians)
        float spinSpeed = 0.0f;   // Current rotary barrel angular velocity (rad/s)

        bool IsValid() const { return data.type != WeaponType::None; }
        float GetAmmoRatio() const
        {
            if (data.maxAmmo <= 0) return 0.0f;
            if (isReloading)
            {
                return std::clamp(1.0f - (reloadTimer / data.reloadDuration), 0.0f, 1.0f);
            }
            return static_cast<float>(currentAmmo) / static_cast<float>(data.maxAmmo);
        }
    };

    struct Projectile
    {
        XMFLOAT3 position;
        XMFLOAT3 prevPosition;
        XMFLOAT3 velocity;
        float lifetime = 2.0f;
        XMFLOAT4 color;
        float damage = 120.0f;
        float impact = 75.0f;
        float directHitMult = 1.65f;

        // Extended homing, splash and projectile properties
        bool isHoming = false;
        XMFLOAT3 homingTarget = { 0.0f, 0.0f, 0.0f };
        float homingTurnRate = 5.5f;
        float homingDelay = 0.15f;
        float splashRadius = 0.0f;
        WeaponType weaponType = WeaponType::KineticRifle;
        uint8_t ownerType = 0; // 0: Local player, 1: Remote player, 2: AI Bot
    };

    enum class ShoulderResult
    {
        None,
        Fired,
        Swapped
    };

    class WeaponSystem
    {
    public:
        WeaponSystem();

        // Catalog & Assemble accessors
        static WeaponData GetCatalogEntry(WeaponType type);
        static const std::vector<WeaponType>& GetAllCatalogTypes();
        static std::vector<WeaponType> GetAvailableTypesForSlot(WeaponSlot slot);

        // Core firing by slot (Returns true if projectile was actually fired)
        bool FireSlot(WeaponSlot slot, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);
        bool FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);
        bool FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);
        bool FireRightBack(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);
        bool FireLeftBack(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);

        void SpawnRemoteProjectile(bool isLeftArm, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, WeaponType type = WeaponType::KineticRifle);
        void SpawnBotProjectile(
            const XMFLOAT3& muzzlePos,
            const XMFLOAT3& targetPos,
            WeaponType type,
            float damage,
            float impact,
            float directHitMult,
            bool isHoming,
            const XMFLOAT3& homingTarget = { 0.0f, 0.0f, 0.0f }
        );

        // Shoulder action: Fires missile if BackOnlyWeapon, otherwise Swaps weapon with arm if ArmWeapon
        ShoulderResult TriggerLeftShoulder(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);
        ShoulderResult TriggerRightShoulder(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio = nullptr, const XMFLOAT3* lockTargetPos = nullptr);

        // Hanger weapon swap (AC6 style: Left swap via Q/LB, Right swap via E/RB)
        bool SwapLeftHanger(AudioManager* audio = nullptr);
        bool SwapRightHanger(AudioManager* audio = nullptr);

        // Assemble weapon cycle (cycle next/previous weapon in catalog for given slot)
        void CycleSlotWeapon(WeaponSlot slot, int direction, AudioManager* audio = nullptr);

        // Loadout preset switching (1 to 5)
        void LoadPreset(int presetIndex, AudioManager* audio = nullptr);
        int GetCurrentPreset() const { return m_currentPreset; }
        std::string GetCurrentPresetName() const;

        // Laser blade state
        bool IsBladeSlashing() const { return m_bladeSlashTimer > 0.0f; }
        float GetBladeSlashTimer() const { return m_bladeSlashTimer; }
        bool IsBladeRightArm() const { return m_isBladeRightArm; }

        // Manual reload triggers
        void ReloadSlot(WeaponSlot slot, AudioManager* audio = nullptr);
        void ReloadRight(AudioManager* audio = nullptr) { ReloadSlot(WeaponSlot::RightArm, audio); }
        void ReloadLeft(AudioManager* audio = nullptr)  { ReloadSlot(WeaponSlot::LeftArm, audio); }
        void ReloadBoth(AudioManager* audio = nullptr)  { ReloadRight(audio); ReloadLeft(audio); }

        void Update(
            float deltaTime,
            PhysicsManager* physicsManager,
            std::vector<TargetDummy>& targets,
            AudioManager* audio = nullptr,
            std::vector<RemoteMech>* remoteMechs = nullptr,
            NetworkManager* network = nullptr,
            MechController* localMech = nullptr,
            std::vector<AIBotMech>* aiBots = nullptr);

        const std::vector<Projectile>& GetProjectiles() const { return m_projectiles; }

        // Slot inspection
        const WeaponInstance& GetSlot(WeaponSlot slot) const { return m_slots[static_cast<int>(slot)]; }
        WeaponInstance& GetSlot(WeaponSlot slot) { return m_slots[static_cast<int>(slot)]; }

        // Equip weapon (for assembling / customization)
        void EquipWeapon(WeaponSlot slot, const WeaponData& data);

        // Backward-compatible accessors for existing renderer / HUD
        int GetRightAmmo() const { return m_slots[static_cast<int>(WeaponSlot::RightArm)].currentAmmo; }
        int GetLeftAmmo() const  { return m_slots[static_cast<int>(WeaponSlot::LeftArm)].currentAmmo; }
        int GetMaxAmmo() const   { return m_slots[static_cast<int>(WeaponSlot::RightArm)].data.maxAmmo; }

        bool IsRightReloading() const { return m_slots[static_cast<int>(WeaponSlot::RightArm)].isReloading; }
        bool IsLeftReloading() const  { return m_slots[static_cast<int>(WeaponSlot::LeftArm)].isReloading; }

        float GetRightAmmoRatio() const { return m_slots[static_cast<int>(WeaponSlot::RightArm)].GetAmmoRatio(); }
        float GetLeftAmmoRatio() const  { return m_slots[static_cast<int>(WeaponSlot::LeftArm)].GetAmmoRatio(); }

        // Back / Hanger specific accessors
        bool HasLeftHanger() const { return m_slots[static_cast<int>(WeaponSlot::LeftBack)].IsValid(); }
        bool HasRightHanger() const { return m_slots[static_cast<int>(WeaponSlot::RightBack)].IsValid(); }
        float GetLeftHangerAmmoRatio() const { return m_slots[static_cast<int>(WeaponSlot::LeftBack)].GetAmmoRatio(); }
        float GetRightHangerAmmoRatio() const { return m_slots[static_cast<int>(WeaponSlot::RightBack)].GetAmmoRatio(); }

    private:
        void UpdateBladeSlash(
            std::vector<TargetDummy>& targets,
            AudioManager* audio,
            std::vector<RemoteMech>* remoteMechs,
            NetworkManager* network,
            MechController* localMech,
            std::vector<AIBotMech>* aiBots
        );

        std::array<WeaponInstance, 4> m_slots;
        std::vector<Projectile> m_projectiles;
        int m_currentPreset = 1;
        float m_bladeSlashTimer = 0.0f;
        bool m_isBladeRightArm = false;
        std::vector<int> m_bladeHitDummies;
        std::vector<uint8_t> m_bladeHitRemotes;
        std::vector<uint8_t> m_bladeHitBots;
    };
}
