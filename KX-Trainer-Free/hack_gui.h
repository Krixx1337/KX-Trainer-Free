#pragma once

#include <string>
#include <vector>

// Forward declare Hack class
class Hack;

class HackGUI {
public:
    HackGUI(Hack& hack);

    // Renders the ImGui UI. Returns true if exit is requested.
    bool renderUI();

private:
    Hack& m_hack; // Reference to the core application logic

    // Feature states managed by the GUI
    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = true; // Default sprint key check ON
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;

    // Hold/Toggle key status states
    bool m_superSprintEnabled = false;
    bool m_flyEnabled = false;
    bool m_sprintActive = false;

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
    void RenderKeysStatusSection();
    void RenderHotkeysSection();
    void RenderLogSection();
    void RenderInfoSection();
    // ----------------------------------

    // --- Private Logic Handling Methods ---
    void HandleSprintToggle();
    void HandleHotkeyRebinding();
    // ------------------------------------

    // Helper function declarations
    const char* GetKeyName(int vk_code);
    void CheckAndSetHotkey(int hotkey_index, const char* name, int& key_variable);
};