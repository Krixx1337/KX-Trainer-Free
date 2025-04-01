#pragma once

#include <functional>
#include <utility>

class Hack; // Forward declaration needed for the action function signature

enum class HotkeyID {
    NONE = -1, // Represents no hotkey being rebound
    SAVE_POS,
    LOAD_POS,
    TOGGLE_INVISIBILITY,
    TOGGLE_WALLCLIMB,
    TOGGLE_CLIPPING,
    TOGGLE_OBJECT_CLIPPING,
    TOGGLE_FULL_STRAFE,
    TOGGLE_NO_FOG,
    HOLD_SUPER_SPRINT,
    TOGGLE_SPRINT_PREF, // Hotkey to toggle the m_sprintEnabled preference flag
    HOLD_FLY
};

enum class HotkeyTriggerType {
    ON_PRESS, // Trigger once when key goes down
    ON_HOLD   // Trigger continuously while key is held
};

struct HotkeyInfo {
    HotkeyID id;
    const char* name;           // User-visible name (e.g., "Save Position")
    int defaultKeyCode;     // Default VK code from Constants::Hotkeys
    int currentKeyCode;     // Currently assigned VK code (0 if unbound)
    HotkeyTriggerType triggerType;
    std::function<void(Hack&, bool)> action; // bool: true=pressed/held

    // Default constructor needed for vector initialization if not using emplace_back with all args
    HotkeyInfo() : id(HotkeyID::NONE), name(""), defaultKeyCode(0), currentKeyCode(0), triggerType(HotkeyTriggerType::ON_PRESS), action(nullptr) {}

    // Constructor for easier initialization
    HotkeyInfo(HotkeyID _id, const char* _name, int _defaultKey, HotkeyTriggerType _type, std::function<void(Hack&, bool)> _action)
        : id(_id), name(_name), defaultKeyCode(_defaultKey), currentKeyCode(0), // Default to unbound (0) initially
          triggerType(_type), action(std::move(_action)) {}
};
