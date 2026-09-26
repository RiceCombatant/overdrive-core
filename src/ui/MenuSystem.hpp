#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Overdrive
{
    class AudioManager;

    struct MenuItem
    {
        std::string id;              // Unique identifier ("training", "host", "client", "exit", etc.)
        std::string label;           // Display label ("TRAINING MODE")
        std::string description;     // Description shown below ("ENTER COMBAT SIMULATION & PRACTICE GROUND")
        std::function<void()> action;// Action executed when selected/confirmed
        bool isEnabled = true;       // Whether item can be selected
    };

    class MenuSystem
    {
    public:
        MenuSystem();
        ~MenuSystem() = default;

        // Add a new menu item (makes adding future modes effortless)
        void AddItem(const std::string& id, const std::string& label, const std::string& description, std::function<void()> action, bool isEnabled = true);

        // Clear all items
        void ClearItems();

        // Navigation
        void NavigateUp(AudioManager* audio = nullptr);
        void NavigateDown(AudioManager* audio = nullptr);
        void Confirm(AudioManager* audio = nullptr);

        // Update animation/pulsing timers
        void Update(float deltaTime);

        // State accessors
        int GetSelectedIndex() const { return m_selectedIndex; }
        const std::vector<MenuItem>& GetItems() const { return m_items; }
        const MenuItem* GetSelectedItem() const;
        float GetAnimTime() const { return m_animTime; }

    private:
        std::vector<MenuItem> m_items;
        int m_selectedIndex = 0;
        float m_animTime = 0.0f;
    };
}
