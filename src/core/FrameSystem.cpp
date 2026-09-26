#include "FrameSystem.hpp"
#include "MechController.hpp"
#include "audio/AudioManager.hpp"
#include <algorithm>
#include <iostream>

namespace Overdrive
{
    FramePartData FrameSystem::GetCatalogEntry(FramePartType type)
    {
        FramePartData p;
        switch (type)
        {
        // ----------------------------------------------------
        // HEAD UNITS
        // ----------------------------------------------------
        case FramePartType::HeadStandard:
            p.type = FramePartType::HeadStandard;
            p.slot = FrameSlot::Head;
            p.id = "HD-012";
            p.name = "HD-012 VEGA";
            p.shortName = "HD-VEGA";
            p.description = "BALANCED VISOR HEAD // STANDARD SENSOR SUITE";
            p.ap = 450.0f;
            p.weight = 180.0f;
            p.energyLoad = 120.0f;
            p.energyCapacity = 50.0f;
            p.aimSpeed = 1.0f;
            p.attitudeStability = 220.0f;
            p.primaryColor = { 0.85f, 0.28f, 0.48f, 1.0f };
            p.frameColor   = { 0.18f, 0.20f, 0.24f, 1.0f };
            p.armorColor   = { 0.92f, 0.94f, 0.96f, 1.0f };
            break;

        case FramePartType::HeadLight:
            p.type = FramePartType::HeadLight;
            p.slot = FrameSlot::Head;
            p.id = "HD-099";
            p.name = "HD-099 AERO";
            p.shortName = "HD-AERO";
            p.description = "AERODYNAMIC STREAMLINE HEAD // MINIMAL MASS & AIR RESISTANCE";
            p.ap = 320.0f;
            p.weight = 105.0f;
            p.energyLoad = 80.0f;
            p.energyCapacity = 0.0f;
            p.aimSpeed = 1.25f; // Fast lock/tracking
            p.speedMod = 1.05f;
            p.attitudeStability = 160.0f;
            p.primaryColor = { 0.25f, 0.75f, 0.95f, 1.0f };
            p.frameColor   = { 0.15f, 0.16f, 0.20f, 1.0f };
            p.armorColor   = { 0.95f, 0.98f, 1.00f, 1.0f };
            break;

        case FramePartType::HeadHeavy:
            p.type = FramePartType::HeadHeavy;
            p.slot = FrameSlot::Head;
            p.id = "HD-044";
            p.name = "HD-044 TITAN";
            p.shortName = "HD-TITN";
            p.description = "HEAVY BLAST-SHIELD VISOR // MAXIMUM ARMOR & STABILITY";
            p.ap = 680.0f;
            p.weight = 340.0f;
            p.energyLoad = 160.0f;
            p.energyCapacity = 80.0f;
            p.aimSpeed = 0.90f;
            p.attitudeStability = 320.0f;
            p.primaryColor = { 0.82f, 0.15f, 0.18f, 1.0f };
            p.frameColor   = { 0.14f, 0.15f, 0.18f, 1.0f };
            p.armorColor   = { 0.72f, 0.74f, 0.78f, 1.0f };
            break;

        case FramePartType::HeadPaladin:
            p.type = FramePartType::HeadPaladin;
            p.slot = FrameSlot::Head;
            p.id = "HD-077";
            p.name = "HD-077 PALADIN";
            p.shortName = "HD-PALD";
            p.description = "COMMANDER BLADE ANTENNA // HIGH EN GENERATOR COUPLING";
            p.ap = 420.0f;
            p.weight = 165.0f;
            p.energyLoad = 140.0f;
            p.energyCapacity = 130.0f;
            p.aimSpeed = 1.10f;
            p.attitudeStability = 210.0f;
            p.primaryColor = { 0.95f, 0.72f, 0.15f, 1.0f };
            p.frameColor   = { 0.18f, 0.18f, 0.22f, 1.0f };
            p.armorColor   = { 0.88f, 0.90f, 0.92f, 1.0f };
            break;

        // ----------------------------------------------------
        // CORE UNITS
        // ----------------------------------------------------
        case FramePartType::CoreStandard:
            p.type = FramePartType::CoreStandard;
            p.slot = FrameSlot::Core;
            p.id = "CR-020";
            p.name = "CR-020 ORBITER";
            p.shortName = "CR-ORBT";
            p.description = "ALL-ROUND MAIN CORE // BALANCED THRUSTER & INTERNAL BAY";
            p.ap = 1450.0f;
            p.weight = 850.0f;
            p.energyLoad = 250.0f;
            p.energyCapacity = 300.0f;
            p.speedMod = 1.0f;
            p.qbMod = 1.0f;
            p.attitudeStability = 450.0f;
            p.primaryColor = { 0.85f, 0.28f, 0.48f, 1.0f };
            p.frameColor   = { 0.18f, 0.20f, 0.24f, 1.0f };
            p.armorColor   = { 0.92f, 0.94f, 0.96f, 1.0f };
            break;

        case FramePartType::CoreLight:
            p.type = FramePartType::CoreLight;
            p.slot = FrameSlot::Core;
            p.id = "CR-088";
            p.name = "CR-088 PHANTOM";
            p.shortName = "CR-PNTM";
            p.description = "LIGHTWEIGHT STREAM CORE // DUAL INTAKE AGILITY CHASSIS";
            p.ap = 1050.0f;
            p.weight = 560.0f;
            p.energyLoad = 180.0f;
            p.energyCapacity = 200.0f;
            p.speedMod = 1.15f;
            p.qbMod = 1.18f;
            p.attitudeStability = 320.0f;
            p.primaryColor = { 0.25f, 0.75f, 0.95f, 1.0f };
            p.frameColor   = { 0.15f, 0.16f, 0.20f, 1.0f };
            p.armorColor   = { 0.95f, 0.98f, 1.00f, 1.0f };
            break;

        case FramePartType::CoreHeavy:
            p.type = FramePartType::CoreHeavy;
            p.slot = FrameSlot::Core;
            p.id = "CR-055";
            p.name = "CR-055 GOLIATH";
            p.shortName = "CR-GLTH";
            p.description = "DREADNOUGHT BULK CORE // MASSIVE DUAL REINFORCED PLATES";
            p.ap = 2100.0f;
            p.weight = 1380.0f;
            p.energyLoad = 360.0f;
            p.energyCapacity = 550.0f;
            p.speedMod = 0.88f;
            p.qbMod = 0.88f;
            p.attitudeStability = 680.0f;
            p.primaryColor = { 0.82f, 0.15f, 0.18f, 1.0f };
            p.frameColor   = { 0.14f, 0.15f, 0.18f, 1.0f };
            p.armorColor   = { 0.72f, 0.74f, 0.78f, 1.0f };
            break;

        case FramePartType::CoreTempest:
            p.type = FramePartType::CoreTempest;
            p.slot = FrameSlot::Core;
            p.id = "CR-066";
            p.name = "CR-066 TEMPEST";
            p.shortName = "CR-TMPS";
            p.description = "OVERDRIVE GENERATOR CORE // BOOSTED ASSAULT BURST DRIVE";
            p.ap = 1350.0f;
            p.weight = 780.0f;
            p.energyLoad = 310.0f;
            p.energyCapacity = 450.0f;
            p.speedMod = 1.08f;
            p.qbMod = 1.12f;
            p.attitudeStability = 410.0f;
            p.primaryColor = { 0.95f, 0.72f, 0.15f, 1.0f };
            p.frameColor   = { 0.18f, 0.18f, 0.22f, 1.0f };
            p.armorColor   = { 0.88f, 0.90f, 0.92f, 1.0f };
            break;

        // ----------------------------------------------------
        // ARMS UNITS
        // ----------------------------------------------------
        case FramePartType::ArmsStandard:
            p.type = FramePartType::ArmsStandard;
            p.slot = FrameSlot::Arms;
            p.id = "AM-030";
            p.name = "AM-030 STRIKER";
            p.shortName = "AM-STRK";
            p.description = "MEDIUM MULTI-ROLE ARMS // BALANCED RECOIL DAMPENING";
            p.ap = 650.0f;
            p.weight = 420.0f;
            p.energyLoad = 150.0f;
            p.recoilControl = 1.0f;
            p.aimSpeed = 1.0f;
            p.attitudeStability = 180.0f;
            p.primaryColor = { 0.85f, 0.28f, 0.48f, 1.0f };
            p.frameColor   = { 0.18f, 0.20f, 0.24f, 1.0f };
            p.armorColor   = { 0.92f, 0.94f, 0.96f, 1.0f };
            break;

        case FramePartType::ArmsLight:
            p.type = FramePartType::ArmsLight;
            p.slot = FrameSlot::Arms;
            p.id = "AM-077";
            p.name = "AM-077 VELOX";
            p.shortName = "AM-VELX";
            p.description = "HIGH-SPEED SERVO ARMS // ULTRA-FAST RETICLE TRACKING";
            p.ap = 480.0f;
            p.weight = 270.0f;
            p.energyLoad = 110.0f;
            p.recoilControl = 0.85f;
            p.aimSpeed = 1.35f;
            p.attitudeStability = 120.0f;
            p.primaryColor = { 0.25f, 0.75f, 0.95f, 1.0f };
            p.frameColor   = { 0.15f, 0.16f, 0.20f, 1.0f };
            p.armorColor   = { 0.95f, 0.98f, 1.00f, 1.0f };
            break;

        case FramePartType::ArmsHeavy:
            p.type = FramePartType::ArmsHeavy;
            p.slot = FrameSlot::Arms;
            p.id = "AM-066";
            p.name = "AM-066 COLOSSUS";
            p.shortName = "AM-CLSS";
            p.description = "HEAVY HYDRAULIC ARMS // MAXIMUM RECOIL STABILIZATION";
            p.ap = 980.0f;
            p.weight = 680.0f;
            p.energyLoad = 220.0f;
            p.recoilControl = 1.55f; // Absorbs heavy bazooka/shotgun kick
            p.aimSpeed = 0.85f;
            p.attitudeStability = 280.0f;
            p.primaryColor = { 0.82f, 0.15f, 0.18f, 1.0f };
            p.frameColor   = { 0.14f, 0.15f, 0.18f, 1.0f };
            p.armorColor   = { 0.72f, 0.74f, 0.78f, 1.0f };
            break;

        case FramePartType::ArmsValiant:
            p.type = FramePartType::ArmsValiant;
            p.slot = FrameSlot::Arms;
            p.id = "AM-088";
            p.name = "AM-088 VALIANT";
            p.shortName = "AM-VLNT";
            p.description = "MELEE COMBAT ARMS // INTEGRATED BLADE BOOSTERS";
            p.ap = 620.0f;
            p.weight = 390.0f;
            p.energyLoad = 170.0f;
            p.recoilControl = 1.15f;
            p.aimSpeed = 1.10f;
            p.attitudeStability = 190.0f;
            p.primaryColor = { 0.95f, 0.72f, 0.15f, 1.0f };
            p.frameColor   = { 0.18f, 0.18f, 0.22f, 1.0f };
            p.armorColor   = { 0.88f, 0.90f, 0.92f, 1.0f };
            break;

        // ----------------------------------------------------
        // LEGS UNITS
        // ----------------------------------------------------
        case FramePartType::LegsBipedal:
            p.type = FramePartType::LegsBipedal;
            p.slot = FrameSlot::Legs;
            p.id = "LG-040";
            p.name = "LG-040 BIPEDAL";
            p.shortName = "LG-BIPD";
            p.description = "STANDARD MEDIUM BIPED // SOLID GROUND STABILITY";
            p.ap = 1150.0f;
            p.weight = 1100.0f;
            p.energyLoad = 200.0f;
            p.speedMod = 1.0f;
            p.qbMod = 1.0f;
            p.jumpMod = 1.0f;
            p.attitudeStability = 350.0f;
            p.primaryColor = { 0.85f, 0.28f, 0.48f, 1.0f };
            p.frameColor   = { 0.18f, 0.20f, 0.24f, 1.0f };
            p.armorColor   = { 0.92f, 0.94f, 0.96f, 1.0f };
            break;

        case FramePartType::LegsReverse:
            p.type = FramePartType::LegsReverse;
            p.slot = FrameSlot::Legs;
            p.id = "LG-066";
            p.name = "LG-066 REVERSE-JOINT";
            p.shortName = "LG-REVJ";
            p.description = "SPRING REVERSE-JOINT LEGS // MASSIVE JUMP & HOP IMPULSE";
            p.ap = 880.0f;
            p.weight = 820.0f;
            p.energyLoad = 170.0f;
            p.speedMod = 1.08f;
            p.qbMod = 1.25f;
            p.jumpMod = 1.48f; // High vertical spring hop
            p.attitudeStability = 290.0f;
            p.primaryColor = { 0.25f, 0.75f, 0.95f, 1.0f };
            p.frameColor   = { 0.15f, 0.16f, 0.20f, 1.0f };
            p.armorColor   = { 0.95f, 0.98f, 1.00f, 1.0f };
            break;

        case FramePartType::LegsHeavyQuad:
            p.type = FramePartType::LegsHeavyQuad;
            p.slot = FrameSlot::Legs;
            p.id = "LG-088";
            p.name = "LG-088 QUADRUPED";
            p.shortName = "LG-QUAD";
            p.description = "HEAVY FORTRESS LEGS // SUPERIOR AP & HEAVY WEAPON CAPACITY";
            p.ap = 1750.0f;
            p.weight = 1680.0f;
            p.energyLoad = 280.0f;
            p.speedMod = 0.88f;
            p.qbMod = 0.85f;
            p.jumpMod = 0.85f;
            p.attitudeStability = 580.0f;
            p.primaryColor = { 0.82f, 0.15f, 0.18f, 1.0f };
            p.frameColor   = { 0.14f, 0.15f, 0.18f, 1.0f };
            p.armorColor   = { 0.72f, 0.74f, 0.78f, 1.0f };
            break;

        case FramePartType::LegsSprint:
            p.type = FramePartType::LegsSprint;
            p.slot = FrameSlot::Legs;
            p.id = "LG-055";
            p.name = "LG-055 RUNNER";
            p.shortName = "LG-RUNR";
            p.description = "HIGH-MOBILITY SPRINT LEGS // TOP CRUISING HOVER SPEED";
            p.ap = 950.0f;
            p.weight = 910.0f;
            p.energyLoad = 190.0f;
            p.speedMod = 1.22f;
            p.qbMod = 1.15f;
            p.jumpMod = 1.10f;
            p.attitudeStability = 260.0f;
            p.primaryColor = { 0.95f, 0.72f, 0.15f, 1.0f };
            p.frameColor   = { 0.18f, 0.18f, 0.22f, 1.0f };
            p.armorColor   = { 0.88f, 0.90f, 0.92f, 1.0f };
            break;

        default:
            break;
        }
        return p;
    }

    std::vector<FramePartType> FrameSystem::GetAvailableTypesForSlot(FrameSlot slot)
    {
        switch (slot)
        {
        case FrameSlot::Head:
            return { FramePartType::HeadStandard, FramePartType::HeadLight, FramePartType::HeadHeavy, FramePartType::HeadPaladin };
        case FrameSlot::Core:
            return { FramePartType::CoreStandard, FramePartType::CoreLight, FramePartType::CoreHeavy, FramePartType::CoreTempest };
        case FrameSlot::Arms:
            return { FramePartType::ArmsStandard, FramePartType::ArmsLight, FramePartType::ArmsHeavy, FramePartType::ArmsValiant };
        case FrameSlot::Legs:
            return { FramePartType::LegsBipedal, FramePartType::LegsReverse, FramePartType::LegsHeavyQuad, FramePartType::LegsSprint };
        default:
            return {};
        }
    }

    FrameSystem::FrameSystem()
    {
        // Default to Standard AC Preset (VEGA + ORBITER + STRIKER + REVERSE-JOINT)
        EquipPart(FrameSlot::Head, GetCatalogEntry(FramePartType::HeadStandard));
        EquipPart(FrameSlot::Core, GetCatalogEntry(FramePartType::CoreStandard));
        EquipPart(FrameSlot::Arms, GetCatalogEntry(FramePartType::ArmsStandard));
        EquipPart(FrameSlot::Legs, GetCatalogEntry(FramePartType::LegsReverse));
    }

    void FrameSystem::EquipPart(FrameSlot slot, const FramePartData& part)
    {
        int idx = static_cast<int>(slot);
        if (idx >= 0 && idx < static_cast<int>(FrameSlot::Count))
        {
            m_slots[idx].data = part;
        }
    }

    void FrameSystem::CycleSlotPart(FrameSlot slot, int direction, AudioManager* audio)
    {
        auto available = GetAvailableTypesForSlot(slot);
        if (available.empty()) return;

        int currentIdx = 0;
        const auto& curPart = m_slots[static_cast<int>(slot)];
        for (size_t i = 0; i < available.size(); ++i)
        {
            if (available[i] == curPart.data.type)
            {
                currentIdx = static_cast<int>(i);
                break;
            }
        }

        int count = static_cast<int>(available.size());
        int nextIdx = (currentIdx + direction) % count;
        if (nextIdx < 0) nextIdx += count;

        EquipPart(slot, GetCatalogEntry(available[nextIdx]));

        if (audio)
        {
            audio->PlayEquipChange();
        }

        std::cout << "[ASSEMBLE FRAME] Slot " << static_cast<int>(slot) 
                  << " changed to: " << m_slots[static_cast<int>(slot)].data.name << std::endl;
    }

    float FrameSystem::GetTotalAp() const
    {
        float total = 0.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) total += slot.data.ap;
        }
        return total;
    }

    float FrameSystem::GetTotalWeight() const
    {
        float total = 0.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) total += slot.data.weight;
        }
        return total;
    }

    float FrameSystem::GetMaxEnergy() const
    {
        float baseEn = 800.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) baseEn += slot.data.energyCapacity;
        }
        return baseEn;
    }

    float FrameSystem::GetNormalSpeed() const
    {
        float weightFactor = 2800.0f / std::max(1200.0f, GetTotalWeight());
        float speedMod = 1.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) speedMod *= slot.data.speedMod;
        }
        return 12.0f * speedMod * std::clamp(weightFactor, 0.75f, 1.35f);
    }

    float FrameSystem::GetBoostSpeed() const
    {
        float weightFactor = 2800.0f / std::max(1200.0f, GetTotalWeight());
        float speedMod = 1.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) speedMod *= slot.data.speedMod;
        }
        return 28.0f * speedMod * std::clamp(weightFactor, 0.75f, 1.35f);
    }

    float FrameSystem::GetAbSpeed() const
    {
        float weightFactor = 2800.0f / std::max(1200.0f, GetTotalWeight());
        float speedMod = 1.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) speedMod *= slot.data.speedMod;
        }
        return 65.0f * speedMod * std::clamp(weightFactor, 0.80f, 1.25f);
    }

    float FrameSystem::GetQbSpeed() const
    {
        float weightFactor = 2800.0f / std::max(1200.0f, GetTotalWeight());
        float qbMod = 1.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid()) qbMod *= slot.data.qbMod;
        }
        return 70.0f * qbMod * std::clamp(weightFactor, 0.75f, 1.35f);
    }

    float FrameSystem::GetJumpVelocity() const
    {
        float jumpMod = m_slots[static_cast<int>(FrameSlot::Legs)].data.jumpMod;
        return 15.0f * jumpMod;
    }

    float FrameSystem::GetRecoilControl() const
    {
        return m_slots[static_cast<int>(FrameSlot::Arms)].data.recoilControl;
    }

    float FrameSystem::GetAimServoRate() const
    {
        float rate = 1.0f;
        rate *= m_slots[static_cast<int>(FrameSlot::Head)].data.aimSpeed;
        rate *= m_slots[static_cast<int>(FrameSlot::Arms)].data.aimSpeed;
        return rate;
    }

    float FrameSystem::GetTotalMaxAcs() const
    {
        float total = 0.0f;
        for (const auto& slot : m_slots)
        {
            if (slot.IsValid())
            {
                total += slot.data.attitudeStability;
            }
        }
        return std::max(600.0f, total);
    }

    float FrameSystem::GetKickDamage() const
    {
        float baseDmg = 520.0f;
        FramePartType legType = m_slots[static_cast<int>(FrameSlot::Legs)].data.type;
        if (legType == FramePartType::LegsReverse)
        {
            baseDmg = 680.0f; // High-impulse spring kick
        }
        else if (legType == FramePartType::LegsHeavyQuad)
        {
            baseDmg = 750.0f; // Massive fortress heavy kick
        }
        else if (legType == FramePartType::LegsSprint)
        {
            baseDmg = 480.0f; // Lightweight sprint kick
        }

        // Weight scaling bonus (heavier mechs deal more kinetic impact damage)
        float weightRatio = GetTotalWeight() / 7000.0f;
        return baseDmg * std::clamp(weightRatio, 0.85f, 1.30f);
    }

    float FrameSystem::GetKickImpact() const
    {
        float baseImpact = 740.0f;
        FramePartType legType = m_slots[static_cast<int>(FrameSlot::Legs)].data.type;
        if (legType == FramePartType::LegsReverse)
        {
            baseImpact = 940.0f; // Reverse-joint kick instantly staggers most ACs
        }
        else if (legType == FramePartType::LegsHeavyQuad)
        {
            baseImpact = 880.0f;
        }
        else if (legType == FramePartType::LegsSprint)
        {
            baseImpact = 650.0f;
        }

        float weightRatio = GetTotalWeight() / 7000.0f;
        return baseImpact * std::clamp(weightRatio, 0.85f, 1.25f);
    }

    void FrameSystem::ApplyToMech(MechController& mech) const
    {
        float totalAp    = GetTotalAp();
        float maxEn      = GetMaxEnergy();
        float normSpeed  = GetNormalSpeed();
        float boostSpeed = GetBoostSpeed();
        float abSpeed    = GetAbSpeed();
        float qbSpeed    = GetQbSpeed();
        float jumpVel    = GetJumpVelocity();
        float maxAcs     = GetTotalMaxAcs();

        mech.SetFrameSpecs(totalAp, maxEn, normSpeed, boostSpeed, abSpeed, qbSpeed, jumpVel, maxAcs);
    }
}
