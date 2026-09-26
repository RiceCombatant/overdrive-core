#include "WeaponSystem.hpp"
#include "physics/PhysicsManager.hpp"
#include "audio/AudioManager.hpp"
#include "network/NetworkManager.hpp"
#include "combat/AIBotMech.hpp"
#include "core/MechController.hpp"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <cmath>
#include <iostream>

namespace Overdrive
{
    WeaponData WeaponSystem::GetCatalogEntry(WeaponType type)
    {
        WeaponData w;
        switch (type)
        {
        case WeaponType::KineticRifle:
            w.type = WeaponType::KineticRifle;
            w.id = "RF-024";
            w.name = "RF-024 KINETIC RIFLE";
            w.shortName = "RA-RIFLE";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 30;
            w.fireRate = 0.18f;
            w.reloadDuration = 2.8f;
            w.damage = 120.0f;
            w.projectileSpeed = 280.0f;
            w.projectileColor = { 1.0f, 0.55f, 0.15f, 1.0f }; // Kinetic Orange
            w.modelSize = { 0.20f, 0.25f, 1.25f };
            w.modelColor = { 0.92f, 0.94f, 0.96f, 1.0f };
            w.recoilKick = 0.18f;
            break;

        case WeaponType::BeamRifle:
            w.type = WeaponType::BeamRifle;
            w.id = "EN-010";
            w.name = "EN-010 BEAM RIFLE";
            w.shortName = "LA-BEAM";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 30;
            w.fireRate = 0.18f;
            w.reloadDuration = 2.5f;
            w.damage = 110.0f;
            w.projectileSpeed = 310.0f;
            w.projectileColor = { 0.25f, 0.85f, 1.0f, 1.0f }; // Electric Cyan beam
            w.modelSize = { 0.20f, 0.24f, 1.25f };
            w.modelColor = { 0.20f, 0.22f, 0.26f, 1.0f };
            w.recoilKick = 0.10f;
            break;

        case WeaponType::HeavyPlasma:
            w.type = WeaponType::HeavyPlasma;
            w.id = "PL-040";
            w.name = "PL-040 HEAVY PLASMA";
            w.shortName = "LB-PLAS";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 15;
            w.fireRate = 0.38f;
            w.reloadDuration = 3.2f;
            w.damage = 250.0f;
            w.projectileSpeed = 230.0f;
            w.projectileColor = { 0.88f, 0.20f, 1.00f, 1.0f }; // Violet Plasma
            w.modelSize = { 0.26f, 0.30f, 1.35f };
            w.modelColor = { 0.35f, 0.20f, 0.45f, 1.0f };
            w.recoilKick = 0.28f;
            break;

        case WeaponType::BurstGun:
            w.type = WeaponType::BurstGun;
            w.id = "MG-014";
            w.name = "MG-014 RAPID BURST";
            w.shortName = "RB-MGUN";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 40;
            w.fireRate = 0.10f;
            w.reloadDuration = 2.2f;
            w.damage = 75.0f;
            w.projectileSpeed = 290.0f;
            w.projectileColor = { 1.00f, 0.82f, 0.18f, 1.0f }; // Amber Gold
            w.modelSize = { 0.18f, 0.22f, 1.15f };
            w.modelColor = { 0.38f, 0.35f, 0.32f, 1.0f };
            w.recoilKick = 0.08f;
            break;

        case WeaponType::MissilePod:
            w.type = WeaponType::MissilePod;
            w.id = "ML-080";
            w.name = "ML-080 VERTI-MISSILE";
            w.shortName = "BK-MISL";
            w.category = WeaponCategory::BackOnlyWeapon; // Dedicated Back Weapon (Direct fire from shoulder)
            w.maxAmmo = 24; // 4 missiles x 6 salvos
            w.fireRate = 0.65f;
            w.reloadDuration = 3.5f;
            w.damage = 160.0f; // per missile (x4 = 640 salvo total)
            w.projectileSpeed = 210.0f;
            w.projectileColor = { 1.00f, 0.95f, 0.85f, 1.0f }; // White smoke/flame
            w.modelSize = { 0.38f, 0.42f, 0.85f };
            w.modelColor = { 0.28f, 0.30f, 0.35f, 1.0f };
            w.isHoming = true;
            w.pelletCount = 4;
            w.splashRadius = 5.5f;
            w.recoilKick = 0.14f;
            break;

        case WeaponType::HeavyBazooka:
            w.type = WeaponType::HeavyBazooka;
            w.id = "BZ-033";
            w.name = "BZ-033 TITAN BAZOOKA";
            w.shortName = "AR-BAZK";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 10;
            w.fireRate = 0.90f;
            w.reloadDuration = 3.8f;
            w.damage = 520.0f; // Massive single shot explosion
            w.projectileSpeed = 165.0f;
            w.projectileColor = { 1.00f, 0.38f, 0.08f, 1.0f }; // Blazing Fire Orange
            w.modelSize = { 0.32f, 0.36f, 1.65f };
            w.modelColor = { 0.22f, 0.24f, 0.26f, 1.0f };
            w.splashRadius = 8.5f;
            w.recoilKick = 0.48f;
            break;

        case WeaponType::LaserBlade:
            w.type = WeaponType::LaserBlade;
            w.id = "LB-077";
            w.name = "LB-077 MOONLIGHT BLADE";
            w.shortName = "AR-BLAD";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 1;
            w.fireRate = 0.85f;
            w.reloadDuration = 1.6f;
            w.damage = 680.0f; // Devastating close-range cleave
            w.impact = 580.0f; // Heavy ACS overload
            w.directHitMult = 2.50f; // Critical stagger direct hit multiplier
            w.projectileSpeed = 0.0f;
            w.projectileColor = { 0.15f, 0.95f, 0.65f, 1.0f }; // Moonlight Green
            w.modelSize = { 0.18f, 0.24f, 0.85f };
            w.modelColor = { 0.15f, 0.25f, 0.22f, 1.0f };
            w.isMelee = true;
            w.recoilKick = 0.05f;
            break;

        case WeaponType::GatlingGun:
            w.type = WeaponType::GatlingGun;
            w.id = "GT-090";
            w.name = "GT-090 VULCAN GATLING";
            w.shortName = "AR-GATL";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 200;
            w.fireRate = 0.05f; // 20 rounds per second
            w.reloadDuration = 4.0f;
            w.damage = 45.0f;
            w.projectileSpeed = 320.0f;
            w.projectileColor = { 1.00f, 0.70f, 0.15f, 1.0f }; // Amber-yellow rapid stream
            w.modelSize = { 0.28f, 0.32f, 1.40f };
            w.modelColor = { 0.25f, 0.26f, 0.28f, 1.0f };
            w.recoilKick = 0.045f;
            break;

        case WeaponType::SpreadShotgun:
            w.type = WeaponType::SpreadShotgun;
            w.id = "SG-020";
            w.name = "SG-020 BREAKER SHOTGUN";
            w.shortName = "AR-SHOT";
            w.category = WeaponCategory::ArmWeapon;
            w.maxAmmo = 16;
            w.fireRate = 0.55f;
            w.reloadDuration = 3.0f;
            w.damage = 65.0f; // x8 pellets = 520 point-blank
            w.projectileSpeed = 270.0f;
            w.projectileColor = { 1.00f, 0.60f, 0.25f, 1.0f };
            w.modelSize = { 0.22f, 0.28f, 1.10f };
            w.modelColor = { 0.30f, 0.28f, 0.26f, 1.0f };
            w.pelletCount = 8;
            w.spreadAngle = 0.085f;
            w.recoilKick = 0.38f;
            break;

        default:
            break;
        }
        return w;
    }

    const std::vector<WeaponType>& WeaponSystem::GetAllCatalogTypes()
    {
        static const std::vector<WeaponType> s_allTypes = {
            WeaponType::KineticRifle,
            WeaponType::BeamRifle,
            WeaponType::HeavyPlasma,
            WeaponType::BurstGun,
            WeaponType::MissilePod,
            WeaponType::HeavyBazooka,
            WeaponType::LaserBlade,
            WeaponType::GatlingGun,
            WeaponType::SpreadShotgun
        };
        return s_allTypes;
    }

    std::vector<WeaponType> WeaponSystem::GetAvailableTypesForSlot(WeaponSlot slot)
    {
        const auto& all = GetAllCatalogTypes();
        std::vector<WeaponType> res;
        for (auto t : all)
        {
            auto data = GetCatalogEntry(t);
            // Arms can only equip ArmWeapon (missile pod is BackOnlyWeapon)
            if (slot == WeaponSlot::RightArm || slot == WeaponSlot::LeftArm)
            {
                if (data.category == WeaponCategory::ArmWeapon)
                {
                    res.push_back(t);
                }
            }
            else // RightBack, LeftBack
            {
                // Back can equip both BackOnlyWeapon and ArmWeapon (hanger)
                res.push_back(t);
            }
        }
        return res;
    }

    // Free helper alias for internal compatibility
    WeaponData GetWeaponCatalogEntry(WeaponType type)
    {
        return WeaponSystem::GetCatalogEntry(type);
    }

    WeaponSystem::WeaponSystem()
    {
        LoadPreset(1, nullptr);
    }

    void WeaponSystem::LoadPreset(int presetIndex, AudioManager* audio)
    {
        m_currentPreset = std::clamp(presetIndex, 1, 5);

        switch (m_currentPreset)
        {
        case 1:
            // 1. STANDARD ASSAULT (Balanced kinetic & energy)
            EquipWeapon(WeaponSlot::RightArm,  GetWeaponCatalogEntry(WeaponType::KineticRifle));
            EquipWeapon(WeaponSlot::LeftArm,   GetWeaponCatalogEntry(WeaponType::BeamRifle));
            EquipWeapon(WeaponSlot::RightBack, GetWeaponCatalogEntry(WeaponType::BurstGun));
            EquipWeapon(WeaponSlot::LeftBack,  GetWeaponCatalogEntry(WeaponType::HeavyPlasma));
            break;

        case 2:
            // 2. HEAVY SIEGE (Heavy Bazooka, Heavy Plasma, Missile Pod, Gatling Gun)
            EquipWeapon(WeaponSlot::RightArm,  GetWeaponCatalogEntry(WeaponType::HeavyBazooka));
            EquipWeapon(WeaponSlot::LeftArm,   GetWeaponCatalogEntry(WeaponType::HeavyPlasma));
            EquipWeapon(WeaponSlot::RightBack, GetWeaponCatalogEntry(WeaponType::MissilePod));
            EquipWeapon(WeaponSlot::LeftBack,  GetWeaponCatalogEntry(WeaponType::GatlingGun));
            break;

        case 3:
            // 3. CLOSE-QUARTERS MELEE (Spread Shotgun, Moonlight Laser Blade, Missile Pod, Kinetic Rifle)
            EquipWeapon(WeaponSlot::RightArm,  GetWeaponCatalogEntry(WeaponType::SpreadShotgun));
            EquipWeapon(WeaponSlot::LeftArm,   GetWeaponCatalogEntry(WeaponType::LaserBlade));
            EquipWeapon(WeaponSlot::RightBack, GetWeaponCatalogEntry(WeaponType::MissilePod));
            EquipWeapon(WeaponSlot::LeftBack,  GetWeaponCatalogEntry(WeaponType::KineticRifle));
            break;

        case 4:
            // 4. BULLET STORM (Dual Gatling Guns, Missile Pod, Rapid Burst)
            EquipWeapon(WeaponSlot::RightArm,  GetWeaponCatalogEntry(WeaponType::GatlingGun));
            EquipWeapon(WeaponSlot::LeftArm,   GetWeaponCatalogEntry(WeaponType::GatlingGun));
            EquipWeapon(WeaponSlot::RightBack, GetWeaponCatalogEntry(WeaponType::MissilePod));
            EquipWeapon(WeaponSlot::LeftBack,  GetWeaponCatalogEntry(WeaponType::BurstGun));
            break;

        case 5:
            // 5. SWARM ARTILLERY (Heavy Bazooka, Shotgun, Dual Missile Pods)
            EquipWeapon(WeaponSlot::RightArm,  GetWeaponCatalogEntry(WeaponType::HeavyBazooka));
            EquipWeapon(WeaponSlot::LeftArm,   GetWeaponCatalogEntry(WeaponType::SpreadShotgun));
            EquipWeapon(WeaponSlot::RightBack, GetWeaponCatalogEntry(WeaponType::MissilePod));
            EquipWeapon(WeaponSlot::LeftBack,  GetWeaponCatalogEntry(WeaponType::MissilePod));
            break;
        }

        if (audio)
        {
            audio->PlayEquipChange();
        }

        std::cout << "[LOADOUT] Switched to Preset " << m_currentPreset << ": " << GetCurrentPresetName() << std::endl;
    }

    std::string WeaponSystem::GetCurrentPresetName() const
    {
        switch (m_currentPreset)
        {
        case 1: return "STANDARD ASSAULT";
        case 2: return "HEAVY SIEGE";
        case 3: return "CQC MELEE";
        case 4: return "BULLET STORM";
        case 5: return "SWARM ARTILLERY";
        default: return "CUSTOM LOADOUT";
        }
    }

    void WeaponSystem::EquipWeapon(WeaponSlot slot, const WeaponData& data)
    {
        int idx = static_cast<int>(slot);
        if (idx < 0 || idx >= static_cast<int>(WeaponSlot::Count)) return;

        m_slots[idx].data = data;
        m_slots[idx].currentAmmo = data.maxAmmo;
        m_slots[idx].cooldown = 0.0f;
        m_slots[idx].reloadTimer = 0.0f;
        m_slots[idx].isReloading = false;
    }

    bool WeaponSystem::SwapLeftHanger(AudioManager* audio)
    {
        auto& arm = m_slots[static_cast<int>(WeaponSlot::LeftArm)];
        auto& back = m_slots[static_cast<int>(WeaponSlot::LeftBack)];

        if (!arm.IsValid() && !back.IsValid()) return false;

        std::swap(arm, back);

        if (audio)
        {
            audio->PlayWeaponSwap({ -0.8f, 1.8f, 0.0f });
        }

        std::cout << "[WEAPON] Left Arm Hanger Swap: Now equipping " << arm.data.name << std::endl;
        return true;
    }

    bool WeaponSystem::SwapRightHanger(AudioManager* audio)
    {
        auto& arm = m_slots[static_cast<int>(WeaponSlot::RightArm)];
        auto& back = m_slots[static_cast<int>(WeaponSlot::RightBack)];

        if (!arm.IsValid() && !back.IsValid()) return false;

        std::swap(arm, back);

        if (audio)
        {
            audio->PlayWeaponSwap({ 0.8f, 1.8f, 0.0f });
        }

        std::cout << "[WEAPON] Right Arm Hanger Swap: Now equipping " << arm.data.name << std::endl;
        return true;
    }

    bool WeaponSystem::FireSlot(WeaponSlot slot, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        int idx = static_cast<int>(slot);
        if (idx < 0 || idx >= static_cast<int>(WeaponSlot::Count)) return false;

        auto& weapon = m_slots[idx];
        if (!weapon.IsValid() || weapon.isReloading || weapon.currentAmmo <= 0 || weapon.cooldown > 0.0f) return false;

        weapon.cooldown = weapon.data.fireRate;
        weapon.currentAmmo--;

        if (weapon.data.type == WeaponType::GatlingGun)
        {
            weapon.spinSpeed = std::min(60.0f, weapon.spinSpeed + 16.0f);
        }

        bool isLeft = (slot == WeaponSlot::LeftArm || slot == WeaponSlot::LeftBack);

        // 1. Play weapon-specific audio
        if (audio)
        {
            switch (weapon.data.type)
            {
            case WeaponType::HeavyBazooka:
                audio->PlayShootBazooka(muzzlePos);
                break;
            case WeaponType::MissilePod:
                audio->PlayShootMissile(muzzlePos);
                break;
            case WeaponType::GatlingGun:
                audio->PlayShootGatling(muzzlePos);
                break;
            case WeaponType::LaserBlade:
                audio->PlayBladeSlash(muzzlePos);
                break;
            case WeaponType::SpreadShotgun:
                audio->PlayShootShotgun(muzzlePos);
                break;
            default:
                if (isLeft)
                {
                    audio->PlayShootLeft(muzzlePos);
                }
                else
                {
                    audio->PlayShootRight(muzzlePos);
                }
                break;
            }
        }

        // 2. Melee weapon execution
        if (weapon.data.isMelee)
        {
            m_bladeSlashTimer = 0.38f;
            m_isBladeRightArm = (slot == WeaponSlot::RightArm);
            m_bladeHitDummies.clear();
            m_bladeHitRemotes.clear();
            m_bladeHitBots.clear();
            ReloadSlot(slot, audio);
            return true;
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

        // 3. Multi-pellet Spread Shotgun
        if (weapon.data.type == WeaponType::SpreadShotgun)
        {
            int pellets = weapon.data.pelletCount > 0 ? weapon.data.pelletCount : 8;
            for (int i = 0; i < pellets; ++i)
            {
                float spreadX = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * weapon.data.spreadAngle * 2.0f;
                float spreadY = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * weapon.data.spreadAngle * 2.0f;

                XMFLOAT3 pDir = { dir.x + spreadX, dir.y + spreadY, dir.z };
                float pLen = std::sqrt(pDir.x * pDir.x + pDir.y * pDir.y + pDir.z * pDir.z);
                if (pLen > 0.001f) { pDir.x /= pLen; pDir.y /= pLen; pDir.z /= pLen; }

                Projectile p;
                p.position     = muzzlePos;
                p.prevPosition = muzzlePos;
                p.velocity     = { pDir.x * weapon.data.projectileSpeed, pDir.y * weapon.data.projectileSpeed, pDir.z * weapon.data.projectileSpeed };
                p.lifetime     = 1.5f;
                p.damage       = weapon.data.damage;
                p.impact       = weapon.data.impact;
                p.directHitMult = weapon.data.directHitMult;
                p.color        = weapon.data.projectileColor;
                p.weaponType   = weapon.data.type;
                m_projectiles.push_back(p);
            }
        }
        // 4. Vertical Micro-Missile Pod Salvo (4 missiles per salvo)
        else if (weapon.data.type == WeaponType::MissilePod)
        {
            XMFLOAT3 homingTarget = lockTargetPos ? *lockTargetPos : targetPos;
            int count = weapon.data.pelletCount > 0 ? weapon.data.pelletCount : 4;
            for (int i = 0; i < count; ++i)
            {
                float angle = i * (XM_2PI / static_cast<float>(count));
                float sideSpread = std::cos(angle) * 16.0f;
                float upThrust   = 38.0f + std::sin(angle) * 8.0f;

                Projectile p;
                p.position     = muzzlePos;
                p.prevPosition = muzzlePos;
                // Pop upward and outward initially, then steer toward target
                p.velocity     = { dir.x * 40.0f + sideSpread, upThrust, dir.z * 40.0f };
                p.lifetime     = 4.5f;
                p.damage       = weapon.data.damage;
                p.impact       = weapon.data.impact;
                p.directHitMult = weapon.data.directHitMult;
                p.color        = weapon.data.projectileColor;
                p.isHoming     = true;
                p.homingTarget = homingTarget;
                p.homingDelay  = 0.12f + i * 0.06f; // Staggered lock-on dive
                p.homingTurnRate = 6.2f;
                p.splashRadius = weapon.data.splashRadius;
                p.weaponType   = weapon.data.type;
                m_projectiles.push_back(p);
            }
        }
        // 5. Heavy Bazooka & Standard Single-round Projectiles
        else
        {
            Projectile p;
            p.position     = muzzlePos;
            p.prevPosition = muzzlePos;
            p.velocity     = { dir.x * weapon.data.projectileSpeed, dir.y * weapon.data.projectileSpeed, dir.z * weapon.data.projectileSpeed };
            p.lifetime     = 2.8f;
            p.damage       = weapon.data.damage;
            p.impact       = weapon.data.impact;
            p.directHitMult = weapon.data.directHitMult;
            p.color        = weapon.data.projectileColor;
            p.splashRadius = weapon.data.splashRadius;
            p.weaponType   = weapon.data.type;
            m_projectiles.push_back(p);
        }

        if (weapon.currentAmmo <= 0)
        {
            ReloadSlot(slot, audio);
        }

        return true;
    }

    bool WeaponSystem::FireRightArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        return FireSlot(WeaponSlot::RightArm, muzzlePos, targetPos, audio, lockTargetPos);
    }

    bool WeaponSystem::FireLeftArm(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        return FireSlot(WeaponSlot::LeftArm, muzzlePos, targetPos, audio, lockTargetPos);
    }

    bool WeaponSystem::FireRightBack(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        return FireSlot(WeaponSlot::RightBack, muzzlePos, targetPos, audio, lockTargetPos);
    }

    bool WeaponSystem::FireLeftBack(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        return FireSlot(WeaponSlot::LeftBack, muzzlePos, targetPos, audio, lockTargetPos);
    }

    ShoulderResult WeaponSystem::TriggerLeftShoulder(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        const auto& back = m_slots[static_cast<int>(WeaponSlot::LeftBack)];
        if (!back.IsValid()) return ShoulderResult::None;

        if (back.data.category == WeaponCategory::BackOnlyWeapon)
        {
            bool fired = FireLeftBack(muzzlePos, targetPos, audio, lockTargetPos);
            return fired ? ShoulderResult::Fired : ShoulderResult::None;
        }
        else
        {
            bool swapped = SwapLeftHanger(audio);
            return swapped ? ShoulderResult::Swapped : ShoulderResult::None;
        }
    }

    ShoulderResult WeaponSystem::TriggerRightShoulder(const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, AudioManager* audio, const XMFLOAT3* lockTargetPos)
    {
        const auto& back = m_slots[static_cast<int>(WeaponSlot::RightBack)];
        if (!back.IsValid()) return ShoulderResult::None;

        if (back.data.category == WeaponCategory::BackOnlyWeapon)
        {
            bool fired = FireRightBack(muzzlePos, targetPos, audio, lockTargetPos);
            return fired ? ShoulderResult::Fired : ShoulderResult::None;
        }
        else
        {
            bool swapped = SwapRightHanger(audio);
            return swapped ? ShoulderResult::Swapped : ShoulderResult::None;
        }
    }

    void WeaponSystem::CycleSlotWeapon(WeaponSlot slot, int direction, AudioManager* audio)
    {
        int slotIdx = static_cast<int>(slot);
        if (slotIdx < 0 || slotIdx >= static_cast<int>(WeaponSlot::Count)) return;

        auto available = GetAvailableTypesForSlot(slot);
        if (available.empty()) return;

        WeaponType currentType = m_slots[slotIdx].data.type;
        int curIndex = 0;
        for (size_t i = 0; i < available.size(); ++i)
        {
            if (available[i] == currentType)
            {
                curIndex = static_cast<int>(i);
                break;
            }
        }

        int nextIndex = (curIndex + direction) % static_cast<int>(available.size());
        if (nextIndex < 0) nextIndex += static_cast<int>(available.size());

        EquipWeapon(slot, GetCatalogEntry(available[nextIndex]));

        if (audio)
        {
            audio->PlayEquipChange();
        }
    }

    void WeaponSystem::ReloadSlot(WeaponSlot slot, AudioManager* audio)
    {
        int idx = static_cast<int>(slot);
        if (idx < 0 || idx >= static_cast<int>(WeaponSlot::Count)) return;

        auto& weapon = m_slots[idx];
        if (!weapon.IsValid() || weapon.isReloading || weapon.currentAmmo >= weapon.data.maxAmmo) return;

        weapon.isReloading = true;
        weapon.reloadTimer = weapon.data.reloadDuration;
    }

    void WeaponSystem::SpawnRemoteProjectile(bool isLeftArm, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos, WeaponType type)
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

        WeaponData wData = GetWeaponCatalogEntry(type);
        float speed = wData.projectileSpeed > 10.0f ? wData.projectileSpeed : 280.0f;
        float damage = wData.damage;
        XMFLOAT4 color = isLeftArm ? XMFLOAT4(1.0f, 0.15f, 0.25f, 1.0f) : wData.projectileColor;

        if (type == WeaponType::SpreadShotgun)
        {
            int pellets = wData.pelletCount > 0 ? wData.pelletCount : 8;
            for (int i = 0; i < pellets; ++i)
            {
                float spreadX = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * wData.spreadAngle * 2.0f;
                float spreadY = ((static_cast<float>(rand()) / RAND_MAX) - 0.5f) * wData.spreadAngle * 2.0f;
                XMFLOAT3 pDir = { dir.x + spreadX, dir.y + spreadY, dir.z };
                float pLen = std::sqrt(pDir.x * pDir.x + pDir.y * pDir.y + pDir.z * pDir.z);
                if (pLen > 0.001f) { pDir.x /= pLen; pDir.y /= pLen; pDir.z /= pLen; }

                Projectile p;
                p.position     = muzzlePos;
                p.prevPosition = muzzlePos;
                p.velocity     = { pDir.x * speed, pDir.y * speed, pDir.z * speed };
                p.lifetime     = 1.5f;
                p.damage       = damage;
                p.color        = color;
                p.weaponType   = type;
                m_projectiles.push_back(p);
            }
        }
        else if (type == WeaponType::MissilePod)
        {
            for (int i = 0; i < 4; ++i)
            {
                float angle = i * (XM_2PI / 4.0f);
                float sideSpread = std::cos(angle) * 16.0f;
                float upThrust   = 38.0f + std::sin(angle) * 8.0f;

                Projectile p;
                p.position     = muzzlePos;
                p.prevPosition = muzzlePos;
                p.velocity     = { dir.x * 40.0f + sideSpread, upThrust, dir.z * 40.0f };
                p.lifetime     = 4.5f;
                p.damage       = damage;
                p.color        = color;
                p.isHoming     = true;
                p.homingTarget = targetPos;
                p.homingDelay  = 0.12f + i * 0.06f;
                p.homingTurnRate = 6.2f;
                p.splashRadius = wData.splashRadius;
                p.weaponType   = type;
                m_projectiles.push_back(p);
            }
        }
        else
        {
            Projectile p;
            p.position     = muzzlePos;
            p.prevPosition = muzzlePos;
            p.velocity     = { dir.x * speed, dir.y * speed, dir.z * speed };
            p.lifetime     = (type == WeaponType::HeavyBazooka) ? 3.0f : 2.0f;
            p.damage       = damage;
            p.color        = color;
            p.splashRadius = wData.splashRadius;
            p.weaponType   = type;
            p.ownerType    = 1;
            m_projectiles.push_back(p);
        }
    }

    void WeaponSystem::SpawnBotProjectile(
        const XMFLOAT3& muzzlePos,
        const XMFLOAT3& targetPos,
        WeaponType type,
        float damage,
        float impact,
        float directHitMult,
        bool isHoming,
        const XMFLOAT3& homingTarget)
    {
        WeaponData wData = GetCatalogEntry(type);
        XMFLOAT3 dir = { targetPos.x - muzzlePos.x, targetPos.y - muzzlePos.y, targetPos.z - muzzlePos.z };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.001f) { dir.x /= len; dir.y /= len; dir.z /= len; }
        else { dir = { 0.0f, 0.0f, 1.0f }; }

        float speed = wData.projectileSpeed;
        XMFLOAT4 color = wData.projectileColor;

        if (type == WeaponType::MissilePod)
        {
            for (int i = 0; i < 4; ++i)
            {
                float angle = i * (XM_2PI / 4.0f);
                float sideSpread = std::cos(angle) * 14.0f;
                float upThrust   = 36.0f + std::sin(angle) * 7.0f;

                Projectile p;
                p.position     = muzzlePos;
                p.prevPosition = muzzlePos;
                p.velocity     = { dir.x * 38.0f + sideSpread, upThrust, dir.z * 38.0f };
                p.lifetime     = 4.5f;
                p.damage       = damage;
                p.impact       = impact;
                p.directHitMult = directHitMult;
                p.color        = color;
                p.isHoming     = true;
                p.homingTarget = homingTarget;
                p.homingDelay  = 0.12f + i * 0.06f;
                p.homingTurnRate = 6.0f;
                p.splashRadius = wData.splashRadius;
                p.weaponType   = type;
                p.ownerType    = 2; // AI Bot
                m_projectiles.push_back(p);
            }
        }
        else
        {
            Projectile p;
            p.position     = muzzlePos;
            p.prevPosition = muzzlePos;
            p.velocity     = { dir.x * speed, dir.y * speed, dir.z * speed };
            p.lifetime     = 2.0f;
            p.damage       = damage;
            p.impact       = impact;
            p.directHitMult = directHitMult;
            p.color        = color;
            p.splashRadius = wData.splashRadius;
            p.weaponType   = type;
            p.ownerType    = 2; // AI Bot
            m_projectiles.push_back(p);
        }
    }

    void WeaponSystem::UpdateBladeSlash(
        std::vector<TargetDummy>& targets,
        AudioManager* audio,
        std::vector<RemoteMech>* remoteMechs,
        NetworkManager* network,
        MechController* localMech,
        std::vector<AIBotMech>* aiBots)
    {
        if (!localMech || localMech->IsDestroyed()) return;

        XMFLOAT3 mechPos = localMech->GetPosition();
        float yaw = localMech->GetYaw();
        float pitch = localMech->GetPitch();

        float sinY = std::sin(yaw);
        float cosY = std::cos(yaw);
        float cosP = std::cos(pitch);
        float sinP = std::sin(pitch);

        XMFLOAT3 fwd3D = { sinY * cosP, -sinP, cosY * cosP };
        XMFLOAT3 fwdHoriz = { sinY, 0.0f, cosY };

        // Blade swing center: positioned ahead of mech in view direction (~3.6m ahead)
        XMFLOAT3 bladeCenter = {
            mechPos.x + fwd3D.x * 3.6f,
            mechPos.y + 0.8f + fwd3D.y * 1.5f,
            mechPos.z + fwd3D.z * 3.6f
        };

        const float bladeRadius = 5.2f;
        const float bladeRadiusSq = bladeRadius * bladeRadius;

        WeaponData bladeData = GetCatalogEntry(WeaponType::LaserBlade);
        float bladeDmg = bladeData.damage;
        float bladeImpact = bladeData.impact;
        float bladeDirectHit = bladeData.directHitMult;

        // 1. Check against Target Dummies
        for (size_t i = 0; i < targets.size(); ++i)
        {
            auto& dummy = targets[i];
            if (!dummy.IsAlive()) continue;

            if (std::find(m_bladeHitDummies.begin(), m_bladeHitDummies.end(), static_cast<int>(i)) != m_bladeHitDummies.end())
            {
                continue;
            }

            XMFLOAT3 dPos = dummy.GetPosition();
            float dx = dPos.x - bladeCenter.x;
            float dy = dPos.y - bladeCenter.y;
            float dz = dPos.z - bladeCenter.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            float toX = dPos.x - mechPos.x;
            float toY = dPos.y - mechPos.y;
            float toZ = dPos.z - mechPos.z;
            float distFromMech = std::sqrt(toX * toX + toY * toY + toZ * toZ);

            bool inRange = false;
            if (distSq <= bladeRadiusSq)
            {
                if (distFromMech < 3.2f)
                {
                    inRange = true; // Point-blank range: hit guaranteed
                }
                else
                {
                    float dotFwd = (toX * fwdHoriz.x + toZ * fwdHoriz.z) / distFromMech;
                    if (dotFwd > 0.12f) // Wide 160-degree frontal cleave arc
                    {
                        inRange = true;
                    }
                }
            }

            if (inRange)
            {
                dummy.TakeDamage(bladeDmg, bladeImpact, bladeDirectHit, audio);
                if (audio)
                {
                    audio->PlayBoostKickHit(dPos);
                }
                m_bladeHitDummies.push_back(static_cast<int>(i));

                std::cout << "[COMBAT] >> LASER BLADE CLEAVE HIT! Target Dummy: "
                          << dummy.GetName() << " (Damage: " << static_cast<int>(bladeDmg)
                          << ", Impact: " << static_cast<int>(bladeImpact) << ") <<" << std::endl;
            }
        }

        // 2. Check against Remote Mechs (Multiplayer)
        if (remoteMechs)
        {
            for (auto& rMech : *remoteMechs)
            {
                if (!rMech.IsAlive()) continue;
                uint8_t pId = rMech.GetPlayerId();
                if (network && pId == network->GetLocalPlayerId()) continue;

                if (std::find(m_bladeHitRemotes.begin(), m_bladeHitRemotes.end(), pId) != m_bladeHitRemotes.end())
                {
                    continue;
                }

                XMFLOAT3 rPos = rMech.GetPosition();
                float dx = rPos.x - bladeCenter.x;
                float dy = rPos.y - bladeCenter.y;
                float dz = rPos.z - bladeCenter.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                float toX = rPos.x - mechPos.x;
                float toY = rPos.y - mechPos.y;
                float toZ = rPos.z - mechPos.z;
                float distFromMech = std::sqrt(toX * toX + toY * toY + toZ * toZ);

                bool inRange = false;
                if (distSq <= bladeRadiusSq)
                {
                    if (distFromMech < 3.2f)
                    {
                        inRange = true;
                    }
                    else
                    {
                        float dotFwd = (toX * fwdHoriz.x + toZ * fwdHoriz.z) / distFromMech;
                        if (dotFwd > 0.12f)
                        {
                            inRange = true;
                        }
                    }
                }

                if (inRange)
                {
                    rMech.TakeDamage(bladeDmg, bladeImpact, bladeDirectHit, audio);
                    rMech.ApplyKnockback(fwdHoriz, 28.0f);
                    if (network)
                    {
                        network->SendHitEvent(pId, bladeDmg, rPos, bladeImpact, bladeDirectHit);
                    }
                    if (audio)
                    {
                        audio->PlayBoostKickHit(rPos);
                    }
                    m_bladeHitRemotes.push_back(pId);

                    std::cout << "[COMBAT] >> LASER BLADE CLEAVE HIT! Remote Player "
                              << static_cast<int>(pId + 1) << " (Damage: " << static_cast<int>(bladeDmg)
                              << ", Impact: " << static_cast<int>(bladeImpact) << ") <<" << std::endl;
                }
            }
        }

        // 3. Check against Autonomous AI Combat Bots
        if (aiBots)
        {
            for (auto& bot : *aiBots)
            {
                if (!bot.IsAlive()) continue;
                uint8_t bId = bot.GetBotId();

                if (std::find(m_bladeHitBots.begin(), m_bladeHitBots.end(), bId) != m_bladeHitBots.end())
                {
                    continue;
                }

                XMFLOAT3 bPos = bot.GetPosition();
                float dx = bPos.x - bladeCenter.x;
                float dy = bPos.y - bladeCenter.y;
                float dz = bPos.z - bladeCenter.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                float toX = bPos.x - mechPos.x;
                float toY = bPos.y - mechPos.y;
                float toZ = bPos.z - mechPos.z;
                float distFromMech = std::sqrt(toX * toX + toY * toY + toZ * toZ);

                bool inRange = false;
                if (distSq <= bladeRadiusSq)
                {
                    if (distFromMech < 3.2f)
                    {
                        inRange = true;
                    }
                    else
                    {
                        float dotFwd = (toX * fwdHoriz.x + toZ * fwdHoriz.z) / distFromMech;
                        if (dotFwd > 0.12f)
                        {
                            inRange = true;
                        }
                    }
                }

                if (inRange)
                {
                    bot.TakeDamage(bladeDmg, bladeImpact, bladeDirectHit, audio);
                    bot.ApplyKnockback(fwdHoriz, 28.0f);
                    if (audio)
                    {
                        audio->PlayBoostKickHit(bPos);
                    }
                    m_bladeHitBots.push_back(bId);

                    std::cout << "[COMBAT] >> LASER BLADE CLEAVE HIT! AI Bot: "
                              << bot.GetName() << " (Damage: " << static_cast<int>(bladeDmg)
                              << ", Impact: " << static_cast<int>(bladeImpact) << ") <<" << std::endl;
                }
            }
        }
    }

    void WeaponSystem::Update(
        float deltaTime,
        PhysicsManager* physicsManager,
        std::vector<TargetDummy>& targets,
        AudioManager* audio,
        std::vector<RemoteMech>* remoteMechs,
        NetworkManager* network,
        MechController* localMech,
        std::vector<AIBotMech>* aiBots)
    {
        if (m_bladeSlashTimer > 0.0f)
        {
            m_bladeSlashTimer -= deltaTime;
            if (localMech)
            {
                UpdateBladeSlash(targets, audio, remoteMechs, network, localMech, aiBots);
            }
            if (m_bladeSlashTimer <= 0.0f)
            {
                m_bladeHitDummies.clear();
                m_bladeHitRemotes.clear();
                m_bladeHitBots.clear();
            }
        }

        // Update all 4 slots (cooldowns, reload timers & rotary barrel spin)
        for (int i = 0; i < static_cast<int>(WeaponSlot::Count); ++i)
        {
            auto& weapon = m_slots[i];
            if (!weapon.IsValid()) continue;

            if (weapon.data.type == WeaponType::GatlingGun)
            {
                weapon.spinAngle += weapon.spinSpeed * deltaTime;
                weapon.spinSpeed = std::max(0.0f, weapon.spinSpeed - deltaTime * 14.0f);
            }

            if (weapon.cooldown > 0.0f) weapon.cooldown -= deltaTime;

            if (weapon.isReloading)
            {
                weapon.reloadTimer -= deltaTime;
                if (weapon.reloadTimer <= 0.0f)
                {
                    weapon.reloadTimer = 0.0f;
                    weapon.isReloading = false;
                    weapon.currentAmmo = weapon.data.maxAmmo;
                    if (audio)
                    {
                        XMFLOAT3 relPos = (i == static_cast<int>(WeaponSlot::RightArm) || i == static_cast<int>(WeaponSlot::RightBack))
                            ? XMFLOAT3(0.6f, 1.2f, 0.5f) : XMFLOAT3(-0.6f, 1.2f, 0.5f);
                        audio->PlayReload(relPos);
                    }
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

            // Homing trajectory steering for micro-missiles
            if (it->isHoming)
            {
                it->homingDelay -= deltaTime;
                if (it->homingDelay <= 0.0f)
                {
                    XMFLOAT3 toTarget = {
                        it->homingTarget.x - it->position.x,
                        it->homingTarget.y - it->position.y,
                        it->homingTarget.z - it->position.z
                    };
                    float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
                    if (dist > 0.8f)
                    {
                        toTarget.x /= dist;
                        toTarget.y /= dist;
                        toTarget.z /= dist;

                        float curSpeed = std::sqrt(it->velocity.x * it->velocity.x + it->velocity.y * it->velocity.y + it->velocity.z * it->velocity.z);
                        if (curSpeed < 30.0f) curSpeed = 210.0f;

                        XMFLOAT3 curDir = { it->velocity.x / curSpeed, it->velocity.y / curSpeed, it->velocity.z / curSpeed };
                        float turnRate = std::min(1.0f, it->homingTurnRate * deltaTime);

                        curDir.x += (toTarget.x - curDir.x) * turnRate;
                        curDir.y += (toTarget.y - curDir.y) * turnRate;
                        curDir.z += (toTarget.z - curDir.z) * turnRate;

                        float newLen = std::sqrt(curDir.x * curDir.x + curDir.y * curDir.y + curDir.z * curDir.z);
                        if (newLen > 0.001f)
                        {
                            it->velocity.x = (curDir.x / newLen) * curSpeed;
                            it->velocity.y = (curDir.y / newLen) * curSpeed;
                            it->velocity.z = (curDir.z / newLen) * curSpeed;
                        }
                    }
                }
            }

            it->prevPosition = it->position;
            it->position.x += it->velocity.x * deltaTime;
            it->position.y += it->velocity.y * deltaTime;
            it->position.z += it->velocity.z * deltaTime;

            bool hasHit = false;

            // 1. Proximity check against RemoteMechs (prevents tunneling at high speed)
            if (remoteMechs)
            {
                for (auto& rMech : *remoteMechs)
                {
                    if (!rMech.IsAlive()) continue;

                    XMFLOAT3 rPos = rMech.GetPosition();
                    float dx = it->position.x - rPos.x;
                    float dy = it->position.y - (rPos.y + 1.0f);
                    float dz = it->position.z - rPos.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    float hitThreshold = (it->splashRadius > 0.0f) ? (2.8f * 2.8f) : (2.2f * 2.2f);
                    if (distSq <= hitThreshold)
                    {
                        hasHit = true;
                        rMech.TakeDamage(it->damage, it->impact, it->directHitMult, audio);
                        if (network)
                        {
                            network->SendHitEvent(rMech.GetPlayerId(), it->damage, it->position, it->impact, it->directHitMult);
                        }
                        if (audio)
                        {
                            audio->PlayExplosion(it->position, (it->splashRadius > 0.0f) ? 1.45f : 1.25f);
                        }
                        break;
                    }
                }
            }

            // 1.5 Direct proximity check against local mech (if fired by AI bot or remote player)
            if (!hasHit && it->ownerType != 0 && localMech && !localMech->IsDestroyed())
            {
                XMFLOAT3 mPos = localMech->GetPosition();
                float dx = it->position.x - mPos.x;
                float dy = it->position.y - (mPos.y + 1.1f);
                float dz = it->position.z - mPos.z;
                if (dx * dx + dy * dy + dz * dz <= 2.2f * 2.2f)
                {
                    hasHit = true;
                    localMech->TakeDamage(it->damage, it->impact, it->directHitMult, &it->prevPosition, audio);
                    if (audio)
                    {
                        audio->PlayExplosion(it->position, (it->splashRadius > 0.0f) ? 1.45f : 1.15f);
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
                    bool hitEntity = false;

                    // (A) Check if hit AI Combat Bot
                    if (aiBots)
                    {
                        for (auto& bot : *aiBots)
                        {
                            if (bot.IsAlive() && !bot.GetBodyID().IsInvalid() && hitResult.mBodyID == bot.GetBodyID())
                            {
                                hitEntity = true;
                                bot.TakeDamage(it->damage, it->impact, it->directHitMult, audio);
                                if (audio)
                                {
                                    audio->PlayExplosion(it->position, 1.35f);
                                }
                                break;
                            }
                        }
                    }

                    // (B) Check if hit remote mech via Jolt BodyID
                    if (!hitEntity && remoteMechs)
                    {
                        for (auto& rMech : *remoteMechs)
                        {
                            if (rMech.IsAlive() && !rMech.GetBodyID().IsInvalid() && hitResult.mBodyID == rMech.GetBodyID())
                            {
                                hitEntity = true;
                                rMech.TakeDamage(it->damage, it->impact, it->directHitMult, audio);
                                if (network)
                                {
                                    network->SendHitEvent(rMech.GetPlayerId(), it->damage, it->position, it->impact, it->directHitMult);
                                }
                                if (audio)
                                {
                                    audio->PlayExplosion(it->position, 1.35f);
                                }
                                break;
                            }
                        }
                    }

                    // (C) Check if hit one of our target dummies
                    if (!hitEntity)
                    {
                        for (auto& target : targets)
                        {
                            if (!target.IsDestroyed() && target.GetBodyID() == hitResult.mBodyID)
                            {
                                target.TakeDamage(it->damage, it->impact, it->directHitMult, audio);
                                if (target.IsDestroyed())
                                {
                                    targetDestroyed = true;
                                }
                                break;
                            }
                        }

                        if (audio)
                        {
                            audio->PlayExplosion(it->position, (it->splashRadius > 0.0f) ? 1.50f : (targetDestroyed ? 1.25f : 0.85f));
                        }
                    }
                }
            }

            // 3. Splash damage explosion propagation for Bazooka and Missiles
            if (hasHit && it->splashRadius > 0.0f)
            {
                // Splash on local player
                if (it->ownerType != 0 && localMech && !localMech->IsDestroyed())
                {
                    XMFLOAT3 mPos = localMech->GetPosition();
                    float dx = it->position.x - mPos.x;
                    float dy = it->position.y - (mPos.y + 1.1f);
                    float dz = it->position.z - mPos.z;
                    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                    if (dist <= it->splashRadius && dist > 0.1f)
                    {
                        float falloff = 1.0f - (dist / it->splashRadius);
                        float splashDmg = it->damage * falloff * 0.70f;
                        float splashImpact = it->impact * falloff * 0.70f;
                        localMech->TakeDamage(splashDmg, splashImpact, it->directHitMult, &it->position, audio);
                    }
                }

                // Splash on AI Bots
                if (aiBots)
                {
                    for (auto& bot : *aiBots)
                    {
                        if (!bot.IsAlive()) continue;
                        XMFLOAT3 bPos = bot.GetPosition();
                        float dx = it->position.x - bPos.x;
                        float dy = it->position.y - (bPos.y + 1.1f);
                        float dz = it->position.z - bPos.z;
                        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                        if (dist <= it->splashRadius && dist > 0.1f)
                        {
                            float falloff = 1.0f - (dist / it->splashRadius);
                            float splashDmg = it->damage * falloff * 0.70f;
                            float splashImpact = it->impact * falloff * 0.70f;
                            bot.TakeDamage(splashDmg, splashImpact, it->directHitMult, audio);
                        }
                    }
                }

                // Splash on remote mechs
                if (remoteMechs)
                {
                    for (auto& rMech : *remoteMechs)
                    {
                        if (!rMech.IsAlive()) continue;
                        XMFLOAT3 rPos = rMech.GetPosition();
                        float dx = it->position.x - rPos.x;
                        float dy = it->position.y - (rPos.y + 1.0f);
                        float dz = it->position.z - rPos.z;
                        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                        if (dist <= it->splashRadius && dist > 0.1f)
                        {
                            float falloff = 1.0f - (dist / it->splashRadius);
                            float splashDmg = it->damage * falloff * 0.70f;
                            float splashImpact = it->impact * falloff * 0.70f;
                            rMech.TakeDamage(splashDmg, splashImpact, it->directHitMult, audio);
                            if (network)
                            {
                                network->SendHitEvent(rMech.GetPlayerId(), splashDmg, it->position, splashImpact, it->directHitMult);
                            }
                        }
                    }
                }

                // Splash on targets
                for (auto& target : targets)
                {
                    if (target.IsDestroyed()) continue;
                    XMFLOAT3 tPos = target.GetPosition();
                    float dx = it->position.x - tPos.x;
                    float dy = it->position.y - tPos.y;
                    float dz = it->position.z - tPos.z;
                    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                    if (dist <= it->splashRadius)
                    {
                        float falloff = 1.0f - (dist / it->splashRadius);
                        float splashDmg = it->damage * falloff * 0.70f;
                        float splashImpact = it->impact * falloff * 0.70f;
                        target.TakeDamage(splashDmg, splashImpact, it->directHitMult, audio);
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
