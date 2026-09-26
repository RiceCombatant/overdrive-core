#include "ui/MenuSystem.hpp"
#include "audio/AudioManager.hpp"

namespace Overdrive
{
    MenuSystem::MenuSystem()
        : m_selectedIndex(0)
        , m_animTime(0.0f)
    {
    }

    void MenuSystem::AddItem(const std::string& id, const std::string& label, const std::string& description, std::function<void()> action, bool isEnabled)
    {
        MenuItem item;
        item.id = id;
        item.label = label;
        item.description = description;
        item.action = std::move(action);
        item.isEnabled = isEnabled;
        m_items.push_back(std::move(item));

        if (m_selectedIndex >= static_cast<int>(m_items.size()))
        {
            m_selectedIndex = 0;
        }
    }

    void MenuSystem::ClearItems()
    {
        m_items.clear();
        m_selectedIndex = 0;
    }

    void MenuSystem::NavigateUp(AudioManager* audio)
    {
        if (m_items.empty()) return;

        int originalIndex = m_selectedIndex;
        int count = static_cast<int>(m_items.size());

        for (int i = 1; i <= count; ++i)
        {
            int nextIdx = (originalIndex - i + count) % count;
            if (m_items[nextIdx].isEnabled)
            {
                m_selectedIndex = nextIdx;
                break;
            }
        }

        if (m_selectedIndex != originalIndex && audio)
        {
            audio->PlayMenuMove();
        }
    }

    void MenuSystem::NavigateDown(AudioManager* audio)
    {
        if (m_items.empty()) return;

        int originalIndex = m_selectedIndex;
        int count = static_cast<int>(m_items.size());

        for (int i = 1; i <= count; ++i)
        {
            int nextIdx = (originalIndex + i) % count;
            if (m_items[nextIdx].isEnabled)
            {
                m_selectedIndex = nextIdx;
                break;
            }
        }

        if (m_selectedIndex != originalIndex && audio)
        {
            audio->PlayMenuMove();
        }
    }

    void MenuSystem::Confirm(AudioManager* audio)
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
        {
            if (m_items[m_selectedIndex].isEnabled && m_items[m_selectedIndex].action)
            {
                if (audio)
                {
                    audio->PlayMenuConfirm();
                }
                m_items[m_selectedIndex].action();
            }
        }
    }

    void MenuSystem::Update(float deltaTime)
    {
        m_animTime += deltaTime;
    }

    const MenuItem* MenuSystem::GetSelectedItem() const
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
        {
            return &m_items[m_selectedIndex];
        }
        return nullptr;
    }
}
