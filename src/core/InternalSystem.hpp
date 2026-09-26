#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <DirectXMath.h>
#include <string>
#include <vector>

namespace Overdrive
{
    using namespace DirectX;

    class MechController;
    class TargetLockSystem;
    class AudioManager;
    class FrameSystem;

    enum class InternalSlot
    {
        Booster = 0,
        Fcs,
        Generator,
        Count
    };

    // 1. BOOSTER: Controls cruise speed, QB impulse, upward lift, and QB energy drain
    struct BoosterPartData
    {
        std::string id;
        std::string name;
        std::string shortName;
        std::string description;
        float weight = 350.0f;           // kg
        float energyLoad = 120.0f;       // EN drain
        float thrustSpeed = 32.0f;       // Cruise boost speed (m/s)
        float qbThrust = 68.0f;          // QuickBoost impulse speed (m/s)
        float qbEnergyCost = 210.0f;     // EN drained per QB
        float upwardThrust = 24.0f;      // Vertical jump/ascend force
    };

    // 2. FCS (Fire Control System): Distance tracking assist & missile lock correction
    // AC6 standard: Close (<130m), Medium (130m-260m), Long (>260m)
    struct FcsPartData
    {
        std::string id;
        std::string name;
        std::string shortName;
        std::string description;
        float weight = 120.0f;           // kg
        float energyLoad = 180.0f;       // EN drain
        float closeAssist = 1.0f;        // Multiplier for <130m (<1.0 slower, >1.0 faster)
        float mediumAssist = 1.0f;       // Multiplier for 130m-260m
        float longAssist = 1.0f;         // Multiplier for >260m
        float missileLockMod = 1.0f;     // Multiplier for missile lock acquisition time
    };

    // 3. GENERATOR: Total EN pool, recharge rate, and delay after EN consumption
    struct GeneratorPartData
    {
        std::string id;
        std::string name;
        std::string shortName;
        std::string description;
        float weight = 700.0f;           // kg
        float energyCapacity = 1400.0f;  // Max EN capacity
        float rechargeRate = 480.0f;     // EN recharge rate per second (ground)
        float rechargeRateAir = 130.0f;  // EN recharge rate per second (airborne)
        float recoveryDelay = 0.40f;     // Seconds delay before EN begins to recharge
    };

    class InternalSystem
    {
    public:
        InternalSystem();

        // Catalog queries
        static std::vector<BoosterPartData> GetBoosterCatalog();
        static std::vector<FcsPartData> GetFcsCatalog();
        static std::vector<GeneratorPartData> GetGeneratorCatalog();

        // Slot inspection
        int GetSelectedPartIndex(InternalSlot slot) const;
        void SetSelectedPartIndex(InternalSlot slot, int index);
        void CycleSlotPart(InternalSlot slot, int direction, AudioManager* audio = nullptr);

        const BoosterPartData& GetCurrentBooster() const;
        const FcsPartData& GetCurrentFcs() const;
        const GeneratorPartData& GetCurrentGenerator() const;

        // Total Internal Specs
        float GetTotalInternalWeight() const;
        float GetTotalInternalEnergyLoad() const;

        // Apply internal tuning to MechController and TargetLockSystem
        void ApplyToMechAndFcs(
            MechController& mech,
            TargetLockSystem& targetLock,
            const FrameSystem& frames
        ) const;

    private:
        int m_boosterIndex = 0;
        int m_fcsIndex = 0;
        int m_generatorIndex = 0;

        std::vector<BoosterPartData> m_boosters;
        std::vector<FcsPartData> m_fcsList;
        std::vector<GeneratorPartData> m_generators;
    };
}
