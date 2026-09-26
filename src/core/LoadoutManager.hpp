#pragma once

#include <string>

namespace Overdrive
{
    class WeaponSystem;
    class FrameSystem;
    class InternalSystem;

    class LoadoutManager
    {
    public:
        // Save current assembled loadout to INI file
        static bool SaveLoadout(
            const std::string& filepath,
            const WeaponSystem& weapons,
            const FrameSystem& frame,
            const InternalSystem& internal
        );

        // Load assembled loadout from INI file and apply to systems
        static bool LoadLoadout(
            const std::string& filepath,
            WeaponSystem& weapons,
            FrameSystem& frame,
            InternalSystem& internal
        );
    };
}
