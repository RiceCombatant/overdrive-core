#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <array>
#include <memory>

namespace Overdrive
{
    using namespace DirectX;

    class MechController;
    class AudioManager;

    enum class FrameSlot : int
    {
        Head  = 0,
        Core  = 1,
        Arms  = 2,
        Legs  = 3,
        Count = 4
    };

    enum class FramePartType
    {
        // 1. Heads
        HeadStandard,   // HD-012 VEGA (Balanced sensor & AP)
        HeadLight,      // HD-099 AERO (Ultra-light aerodynamic, low weight)
        HeadHeavy,      // HD-044 TITAN (Heavy blast visor, maximum AP)
        HeadPaladin,    // HD-077 PALADIN (Blade commander antenna, high EN efficiency)

        // 2. Cores
        CoreStandard,   // CR-020 ORBITER (Balanced frame, standard generator)
        CoreLight,      // CR-088 PHANTOM (High-speed stream core, high agility)
        CoreHeavy,      // CR-055 GOLIATH (Heavy dreadnought bulkhead, massive AP & EN)
        CoreTempest,    // CR-066 TEMPEST (Overdrive core, maximized Assault Boost thrust)

        // 3. Arms
        ArmsStandard,   // AM-030 STRIKER (Balanced arms, steady weapon mount)
        ArmsLight,      // AM-077 VELOX (Lightweight fast servo arms, rapid aim tracking)
        ArmsHeavy,      // AM-066 COLOSSUS (Heavy hydraulic reinforced arms, supreme recoil dampening)
        ArmsValiant,    // AM-088 VALIANT (Melee combat arms with integrated boosters)

        // 4. Legs
        LegsBipedal,    // LG-040 BIPEDAL (Standard medium biped, all-terrain balance)
        LegsReverse,    // LG-066 REVERSE-JOINT (High-jump reverse joint, aerial agility & hop impulse)
        LegsHeavyQuad,  // LG-088 QUADRUPED (Heavy fortress quadruped legs, maximum AP & load)
        LegsSprint,     // LG-055 RUNNER (High-mobility sprint legs, top cruising hover speed)

        None
    };

    struct FramePartData
    {
        FramePartType type = FramePartType::None;
        FrameSlot slot = FrameSlot::Head;
        std::string id;
        std::string name;
        std::string shortName;
        std::string description;

        // Combat & Mech Parameters
        float ap = 500.0f;           // Armor points contributed
        float weight = 300.0f;       // Weight in kg
        float energyLoad = 150.0f;   // EN consumption/load
        float energyCapacity = 0.0f; // Extra EN pool (mainly Core/Head)
        float speedMod = 1.0f;       // Boost speed multiplier
        float qbMod = 1.0f;          // QB thrust multiplier
        float jumpMod = 1.0f;        // Jump thrust multiplier
        float recoilControl = 1.0f;  // Recoil stabilization (Arms)
        float aimSpeed = 1.0f;       // Aim servo tracking rate (Arms/Head)
        float attitudeStability = 220.0f; // ACS Attitude Stability / Stagger resistance

        // Colors for visual distinction
        XMFLOAT4 primaryColor = { 0.85f, 0.28f, 0.48f, 1.0f };
        XMFLOAT4 frameColor   = { 0.18f, 0.20f, 0.24f, 1.0f };
        XMFLOAT4 armorColor   = { 0.92f, 0.94f, 0.96f, 1.0f };
    };

    struct FrameInstance
    {
        FramePartData data;
        bool IsValid() const { return data.type != FramePartType::None; }
    };

    class FrameSystem
    {
    public:
        FrameSystem();

        static FramePartData GetCatalogEntry(FramePartType type);
        static std::vector<FramePartType> GetAvailableTypesForSlot(FrameSlot slot);

        const FrameInstance& GetSlot(FrameSlot slot) const { return m_slots[static_cast<int>(slot)]; }
        FrameInstance& GetSlot(FrameSlot slot) { return m_slots[static_cast<int>(slot)]; }

        void EquipPart(FrameSlot slot, const FramePartData& part);
        void CycleSlotPart(FrameSlot slot, int direction, AudioManager* audio = nullptr);

        // Aggregate mech properties calculated from all 4 equipped frame parts
        float GetTotalAp() const;
        float GetTotalWeight() const;
        float GetMaxEnergy() const;
        float GetTotalMaxAcs() const;
        float GetNormalSpeed() const;
        float GetBoostSpeed() const;
        float GetAbSpeed() const;
        float GetQbSpeed() const;
        float GetJumpVelocity() const;
        float GetRecoilControl() const;
        float GetAimServoRate() const;
        float GetKickDamage() const;
        float GetKickImpact() const;

        // Apply calculated stats to MechController
        void ApplyToMech(MechController& mech) const;

    private:
        std::array<FrameInstance, 4> m_slots;
    };
}
