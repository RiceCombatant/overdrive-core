#include "LoadoutManager.hpp"
#include "combat/WeaponSystem.hpp"
#include "core/FrameSystem.hpp"
#include "core/InternalSystem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace Overdrive
{
    static std::string Trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    bool LoadoutManager::SaveLoadout(
        const std::string& filepath,
        const WeaponSystem& weapons,
        const FrameSystem& frame,
        const InternalSystem& internal)
    {
        std::ofstream out(filepath);
        if (!out.is_open())
        {
            std::cerr << "[LOADOUT] Failed to open file for saving: " << filepath << std::endl;
            return false;
        }

        out << "; Overdrive Core - Assembled Mech Configuration\n";
        out << "; Automatically saved when assembling in Garage\n\n";

        // 1. Weapons Section (4 Slots)
        out << "[Weapons]\n";
        out << "RightArm="  << weapons.GetSlot(WeaponSlot::RightArm).data.id  << "\n";
        out << "LeftArm="   << weapons.GetSlot(WeaponSlot::LeftArm).data.id   << "\n";
        out << "RightBack=" << weapons.GetSlot(WeaponSlot::RightBack).data.id << "\n";
        out << "LeftBack="  << weapons.GetSlot(WeaponSlot::LeftBack).data.id  << "\n\n";

        // 2. Frame Parts Section (4 Slots)
        out << "[Frame]\n";
        out << "Head=" << frame.GetSlot(FrameSlot::Head).data.id << "\n";
        out << "Core=" << frame.GetSlot(FrameSlot::Core).data.id << "\n";
        out << "Arms=" << frame.GetSlot(FrameSlot::Arms).data.id << "\n";
        out << "Legs=" << frame.GetSlot(FrameSlot::Legs).data.id << "\n\n";

        // 3. Internal Parts Section (3 Slots)
        out << "[Internal]\n";
        out << "Booster="   << internal.GetCurrentBooster().id   << "\n";
        out << "FCS="       << internal.GetCurrentFcs().id       << "\n";
        out << "Generator=" << internal.GetCurrentGenerator().id << "\n";

        out.close();
        std::cout << "[LOADOUT] Assembled mech successfully saved to " << filepath << std::endl;
        return true;
    }

    bool LoadoutManager::LoadLoadout(
        const std::string& filepath,
        WeaponSystem& weapons,
        FrameSystem& frame,
        InternalSystem& internal)
    {
        std::ifstream in(filepath);
        if (!in.is_open())
        {
            std::cout << "[LOADOUT] No existing config found at " << filepath << ". Using default build." << std::endl;
            return false;
        }

        std::string currentSection = "";
        std::string line;

        while (std::getline(in, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#')
            {
                continue;
            }

            if (line.front() == '[' && line.back() == ']')
            {
                currentSection = line.substr(1, line.length() - 2);
                currentSection = Trim(currentSection);
                continue;
            }

            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = Trim(line.substr(0, eqPos));
            std::string val = Trim(line.substr(eqPos + 1));

            if (currentSection == "Weapons")
            {
                WeaponSlot slot = WeaponSlot::Count;
                if (key == "RightArm")       slot = WeaponSlot::RightArm;
                else if (key == "LeftArm")   slot = WeaponSlot::LeftArm;
                else if (key == "RightBack") slot = WeaponSlot::RightBack;
                else if (key == "LeftBack")  slot = WeaponSlot::LeftBack;

                if (slot != WeaponSlot::Count)
                {
                    // Find catalog weapon with matching ID
                    for (WeaponType wType : WeaponSystem::GetAllCatalogTypes())
                    {
                        WeaponData entry = WeaponSystem::GetCatalogEntry(wType);
                        if (entry.id == val)
                        {
                            weapons.EquipWeapon(slot, entry);
                            break;
                        }
                    }
                }
            }
            else if (currentSection == "Frame")
            {
                FrameSlot slot = FrameSlot::Count;
                if (key == "Head")       slot = FrameSlot::Head;
                else if (key == "Core")  slot = FrameSlot::Core;
                else if (key == "Arms")  slot = FrameSlot::Arms;
                else if (key == "Legs")  slot = FrameSlot::Legs;

                if (slot != FrameSlot::Count)
                {
                    for (FramePartType pType : FrameSystem::GetAvailableTypesForSlot(slot))
                    {
                        FramePartData entry = FrameSystem::GetCatalogEntry(pType);
                        if (entry.id == val)
                        {
                            frame.EquipPart(slot, entry);
                            break;
                        }
                    }
                }
            }
            else if (currentSection == "Internal")
            {
                if (key == "Booster")
                {
                    const auto& boosters = InternalSystem::GetBoosterCatalog();
                    for (size_t i = 0; i < boosters.size(); ++i)
                    {
                        if (boosters[i].id == val)
                        {
                            internal.SetSelectedPartIndex(InternalSlot::Booster, static_cast<int>(i));
                            break;
                        }
                    }
                }
                else if (key == "FCS")
                {
                    const auto& fcsList = InternalSystem::GetFcsCatalog();
                    for (size_t i = 0; i < fcsList.size(); ++i)
                    {
                        if (fcsList[i].id == val)
                        {
                            internal.SetSelectedPartIndex(InternalSlot::Fcs, static_cast<int>(i));
                            break;
                        }
                    }
                }
                else if (key == "Generator")
                {
                    const auto& gens = InternalSystem::GetGeneratorCatalog();
                    for (size_t i = 0; i < gens.size(); ++i)
                    {
                        if (gens[i].id == val)
                        {
                            internal.SetSelectedPartIndex(InternalSlot::Generator, static_cast<int>(i));
                            break;
                        }
                    }
                }
            }
        }

        std::cout << "[LOADOUT] Successfully loaded assembled mech from " << filepath << std::endl;
        return true;
    }
}
