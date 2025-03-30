#pragma once

#include <string>
#include <vector>

class Hack; // Forward declaration

class HackGUI {
public:
    HackGUI(Hack& hack);
    bool renderUI();

private:
    Hack& m_hack;

    // Feature states
    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = false;
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;

    // Hold key active states
    bool m_superSprintActive = false;
    bool m_flyActive = false;

    // Hotkeys
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

    // UI Rendering Methods
    void RenderAlwaysOnTop();
    void RenderTogglesSection();
    void RenderActionsSection();
    void RenderHotkeysSection();
    void RenderLogSection();
    void RenderInfoSection();

    // Logic Handling Methods
    void HandleHotkeys();
    void HandleHotkeyRebinding();

    // Helpers
    const char* GetKeyName(int vk_code);
    void CheckAndSetHotkey(int hotkey_index, const char* name, int& key_variable);
};