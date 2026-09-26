#include "core/InternalSystem.hpp"
#include "core/MechController.hpp"
#include "core/FrameSystem.hpp"
#include "combat/TargetLockSystem.hpp"
#include "audio/AudioManager.hpp"
#include <algorithm>
#include <iostream>

namespace Overdrive
{
    std::vector<BoosterPartData> InternalSystem::GetBoosterCatalog()
    {
        return {
            {
                "BST-010",
                "BST-010 ALULA",
                "ALULA",
                "HIGH QB SPRINT BOOSTER",
                320.0f, // weight
                240.0f, // EN load
                36.0f,  // Cruise thrust speed
                78.0f,  // QB impulse thrust (Very high)
                260.0f, // QB energy cost
                22.0f   // Upward thrust
            },
            {
                "BST-020",
                "BST-020 GILLS",
                "GILLS",
                "LOW DRAIN BALANCED BOOSTER",
                280.0f, // weight
                150.0f, // EN load
                32.0f,  // Cruise thrust speed
                66.0f,  // QB impulse thrust
                170.0f, // QB energy cost (High efficiency)
                24.0f   // Upward thrust
            },
            {
                "BST-030",
                "BST-030 FLUEGEL",
                "FLUEGEL",
                "VERTICAL COMBAT LIFT BOOSTER",
                340.0f, // weight
                210.0f, // EN load
                30.0f,  // Cruise thrust speed
                62.0f,  // QB impulse thrust
                205.0f, // QB energy cost
                34.0f   // Upward thrust (Maximum vertical leap)
            },
            {
                "BST-040",
                "BST-040 BXT-7",
                "BXT-7",
                "HEAVY BRUTE FORCE THRUSTER",
                460.0f, // weight
                320.0f, // EN load
                38.0f,  // Cruise thrust speed (Powerful forward momentum)
                74.0f,  // QB impulse thrust
                290.0f, // QB energy cost
                26.0f   // Upward thrust
            }
        };
    }

    std::vector<FcsPartData> InternalSystem::GetFcsCatalog()
    {
        return {
            {
                "FCS-010",
                "FCS-010 OCELLUS",
                "OCELLUS",
                "CLOSE-QUARTERS MELEE / SHOTGUN FCS",
                80.0f,  // weight
                160.0f, // EN load
                2.2f,   // Close assist (<130m) - Rapid instant lock
                0.9f,   // Medium assist (130m-260m)
                0.4f,   // Long assist (>260m) - Weak
                1.2f    // Missile lock mod
            },
            {
                "FCS-020",
                "FCS-020 TALBOT",
                "TALBOT",
                "MID-RANGE ASSAULT BALANCED FCS",
                100.0f, // weight
                190.0f, // EN load
                1.5f,   // Close assist
                1.6f,   // Medium assist - Optimized for rifles & pulse
                0.9f,   // Long assist
                1.0f    // Missile lock mod (Standard)
            },
            {
                "FCS-030",
                "FCS-030 WUERGER",
                "WUERGER",
                "LONG-RANGE SNIPER & MISSILE FCS",
                140.0f, // weight
                260.0f, // EN load
                0.5f,   // Close assist - Slow in melee
                1.3f,   // Medium assist
                2.1f,   // Long assist (>260m) - Supreme precision
                0.65f   // Missile lock mod (Ultra-fast missile lock)
            },
            {
                "FCS-040",
                "FCS-040 ABBOT",
                "ABBOT",
                "HIGH-SPEED CQC COMBAT FCS",
                90.0f,  // weight
                175.0f, // EN load
                1.8f,   // Close assist
                1.4f,   // Medium assist
                0.7f,   // Long assist
                0.9f    // Missile lock mod
            }
        };
    }

    std::vector<GeneratorPartData> InternalSystem::GetGeneratorCatalog()
    {
        return {
            {
                "GEN-010",
                "GEN-010 MING-TANG",
                "MING-TANG",
                "HIGH-CYCLE CIRCULATION GENERATOR",
                620.0f,  // weight
                1200.0f, // EN capacity
                620.0f,  // Recharge rate (Ultra-fast recharge)
                180.0f,  // Recharge rate airborne
                0.30f    // Recovery delay (Minimal delay)
            },
            {
                "GEN-020",
                "GEN-020 SAN-TAI",
                "SAN-TAI",
                "SUPER-CAPACITY GENERATOR",
                880.0f,  // weight
                1850.0f, // EN capacity (Huge reservoir for chain QBs)
                440.0f,  // Recharge rate
                120.0f,  // Recharge rate airborne
                0.55f    // Recovery delay
            },
            {
                "GEN-030",
                "GEN-030 HOKUSHI",
                "HOKUSHI",
                "BALANCED HIGH-OUTPUT GENERATOR",
                710.0f,  // weight
                1450.0f, // EN capacity
                520.0f,  // Recharge rate
                150.0f,  // Recharge rate airborne
                0.40f    // Recovery delay
            },
            {
                "GEN-040",
                "GEN-040 CORAL-RED",
                "CORAL-RED",
                "CORAL HIGH-POTENTIAL GENERATOR",
                790.0f,  // weight
                1600.0f, // EN capacity
                390.0f,  // Recharge rate
                110.0f,  // Recharge rate airborne
                0.18f    // Recovery delay (Immediate jumpstart after depletion)
            }
        };
    }

    InternalSystem::InternalSystem()
    {
        m_boosters = GetBoosterCatalog();
        m_fcsList = GetFcsCatalog();
        m_generators = GetGeneratorCatalog();
    }

    int InternalSystem::GetSelectedPartIndex(InternalSlot slot) const
    {
        switch (slot)
        {
        case InternalSlot::Booster:   return m_boosterIndex;
        case InternalSlot::Fcs:       return m_fcsIndex;
        case InternalSlot::Generator: return m_generatorIndex;
        default:                      return 0;
        }
    }

    void InternalSystem::SetSelectedPartIndex(InternalSlot slot, int index)
    {
        switch (slot)
        {
        case InternalSlot::Booster:
            m_boosterIndex = std::clamp(index, 0, static_cast<int>(m_boosters.size()) - 1);
            break;
        case InternalSlot::Fcs:
            m_fcsIndex = std::clamp(index, 0, static_cast<int>(m_fcsList.size()) - 1);
            break;
        case InternalSlot::Generator:
            m_generatorIndex = std::clamp(index, 0, static_cast<int>(m_generators.size()) - 1);
            break;
        default:
            break;
        }
    }

    void InternalSystem::CycleSlotPart(InternalSlot slot, int direction, AudioManager* audio)
    {
        if (slot == InternalSlot::Booster && !m_boosters.empty())
        {
            int count = static_cast<int>(m_boosters.size());
            m_boosterIndex = (m_boosterIndex + direction % count + count) % count;
        }
        else if (slot == InternalSlot::Fcs && !m_fcsList.empty())
        {
            int count = static_cast<int>(m_fcsList.size());
            m_fcsIndex = (m_fcsIndex + direction % count + count) % count;
        }
        else if (slot == InternalSlot::Generator && !m_generators.empty())
        {
            int count = static_cast<int>(m_generators.size());
            m_generatorIndex = (m_generatorIndex + direction % count + count) % count;
        }

        if (audio)
        {
            audio->PlayEquipChange();
        }
    }

    const BoosterPartData& InternalSystem::GetCurrentBooster() const
    {
        return m_boosters[m_boosterIndex];
    }

    const FcsPartData& InternalSystem::GetCurrentFcs() const
    {
        return m_fcsList[m_fcsIndex];
    }

    const GeneratorPartData& InternalSystem::GetCurrentGenerator() const
    {
        return m_generators[m_generatorIndex];
    }

    float InternalSystem::GetTotalInternalWeight() const
    {
        return GetCurrentBooster().weight + GetCurrentFcs().weight + GetCurrentGenerator().weight;
    }

    float InternalSystem::GetTotalInternalEnergyLoad() const
    {
        return GetCurrentBooster().energyLoad + GetCurrentFcs().energyLoad;
    }

    void InternalSystem::ApplyToMechAndFcs(
        MechController& mech,
        TargetLockSystem& targetLock,
        const FrameSystem& frames
    ) const
    {
        const auto& bst = GetCurrentBooster();
        const auto& fcs = GetCurrentFcs();
        const auto& gen = GetCurrentGenerator();

        // 1. Frame baseline properties
        float totalAp = frames.GetTotalAp();
        float totalWeight = frames.GetTotalWeight() + GetTotalInternalWeight();
        
        // Weight multiplier on acceleration (heavy mechs have slightly reduced mobility)
        float weightRatio = totalWeight / 8500.0f; // Normalized around 8500kg
        float mobilityScale = std::clamp(1.20f - 0.20f * weightRatio, 0.75f, 1.25f);

        // Combined cruise and QB speeds
        float finalBoostSpeed = bst.thrustSpeed * mobilityScale;
        float finalQbSpeed    = bst.qbThrust * mobilityScale;
        float finalAbSpeed    = frames.GetAbSpeed() * mobilityScale;
        float finalJumpForce  = bst.upwardThrust * frames.GetSlot(FrameSlot::Legs).data.jumpMod;
        float finalMaxEnergy  = gen.energyCapacity + frames.GetSlot(FrameSlot::Core).data.energyCapacity;

        // Apply frame and mobility specs to mech
        mech.SetFrameSpecs(
            totalAp,
            finalMaxEnergy,
            frames.GetNormalSpeed(),
            finalBoostSpeed,
            finalAbSpeed,
            finalQbSpeed,
            finalJumpForce
        );

        // Apply internal generator / booster tuning to mech
        mech.SetInternalSpecs(
            bst.qbEnergyCost,
            gen.rechargeRate,
            gen.rechargeRateAir,
            gen.recoveryDelay
        );

        // Apply FCS assist modifiers and arm tracking rate to TargetLockSystem
        float armTrackingRate = frames.GetAimServoRate();
        targetLock.SetFcsModifiers(
            fcs.closeAssist,
            fcs.mediumAssist,
            fcs.longAssist,
            fcs.missileLockMod,
            armTrackingRate
        );

        std::cout << "[INTERNAL SYSTEM] Applied Booster: " << bst.name 
                  << ", FCS: " << fcs.name 
                  << ", Generator: " << gen.name << std::endl;
    }
}
