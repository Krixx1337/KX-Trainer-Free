#pragma once

#include <string>
#include <vector>

// Forward declare Hack class
class Hack;

class HackGUI {
public:
    HackGUI(Hack& hack);
    bool renderUI();

private:
    Hack& m_hack; // Reference to the core application logic

    // Feature states synchronized between GUI and Hotkeys
    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = true; // Checkbox state: Is sprint *allowed* by user?
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;

    // Key status states (Hold/Toggle)
    bool m_superSprintActive = false; // Active while key is held
    bool m_flyActive = false;         // Active while key is held
    bool m_sprintActive = false;      // Is sprint currently toggled ON?

    // Modifiable Hotkeys
    int m_key_savepos;
    int m_key_loadpos;
    int m_key_invisibility;
    int m_key_wallclimb;
    int m_key_clipping;
    int m_key_object_clipping;
    int m_key_full_strafe;
    int m_key_no_fog;
    int m_key_super_sprint;
    int m_key_sprint;
    int m_key_fly;

    // Hotkey Rebinding State
    int m_rebinding_hotkey_index = -1;
    const char* m_rebinding_hotkey_name = nullptr;

    // --- Private UI Rendering Methods ---
    void RenderAlwaysOnTop();
    void RenderTogglesSection();
    void RenderActionsSection();
    // Removed: void RenderKeysStatusSection(); // No longer needed
    void RenderHotkeysSection();
    void RenderLogSection();
    void RenderInfoSection(); // Will be simplified

    // --- Private Logic Handling Methods ---
    // Removed: void HandleSprintToggle(); // Merged into HandleHotkeys
    void HandleHotkeys(); // Handles ALL hotkey checks
    void HandleHotkeyRebinding();

    // Helper function declarations
    const char* GetKeyName(int vk_code);
    void CheckAndSetHotkey(int hotkey_index, const char* name, int& key_variable);
};