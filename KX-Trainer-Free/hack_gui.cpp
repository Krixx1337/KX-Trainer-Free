#include "hack_gui.h"
#include "hack.h"
#include "constants.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <cstdio>

extern std::vector<std::string> g_statusMessages;
extern std::mutex g_statusMutex;

//-----------------------------------------------------------------------------
// Helper: Get descriptive names for VK codes
//-----------------------------------------------------------------------------
const char* HackGUI::GetKeyName(int vk_code) {
    if (vk_code == 0) { return "None"; }
    static std::map<int, const char*> keyNames = { /* ... map definition ... */
        {VK_LBUTTON, "LMouse"}, {VK_RBUTTON, "RMouse"}, {VK_MBUTTON, "MMouse"},
        {VK_BACK, "Backspace"}, {VK_TAB, "Tab"}, {VK_RETURN, "Enter"}, {VK_SHIFT, "Shift"},
        {VK_LSHIFT, "LShift"}, {VK_RSHIFT, "RShift"}, {VK_CONTROL, "Ctrl"}, {VK_LCONTROL, "LCtrl"},
        {VK_RCONTROL, "RCtrl"}, {VK_MENU, "Alt"}, {VK_LMENU, "LAlt"}, {VK_RMENU, "RAlt"},
        {VK_PAUSE, "Pause"}, {VK_CAPITAL, "Caps Lock"}, {VK_ESCAPE, "Esc"}, {VK_SPACE, "Space"},
        {VK_PRIOR, "Page Up"}, {VK_NEXT, "Page Down"}, {VK_END, "End"}, {VK_HOME, "Home"},
        {VK_LEFT, "Left Arrow"}, {VK_UP, "Up Arrow"}, {VK_RIGHT, "Right Arrow"}, {VK_DOWN, "Down Arrow"},
        {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"},
        {'0', "0"}, {'1', "1"}, {'2', "2"}, {'3', "3"}, {'4', "4"}, {'5', "5"}, {'6', "6"},
        {'7', "7"}, {'8', "8"}, {'9', "9"}, {'A', "A"}, {'B', "B"}, {'C', "C"}, {'D', "D"},
        {'E', "E"}, {'F', "F"}, {'G', "G"}, {'H', "H"}, {'I', "I"}, {'J', "J"}, {'K', "K"},
        {'L', "L"}, {'M', "M"}, {'N', "N"}, {'O', "O"}, {'P', "P"}, {'Q', "Q"}, {'R', "R"},
        {'S', "S"}, {'T', "T"}, {'U', "U"}, {'V', "V"}, {'W', "W"}, {'X', "X"}, {'Y', "Y"},
        {'Z', "Z"}, {VK_NUMPAD0, "Num 0"}, {VK_NUMPAD1, "Num 1"}, {VK_NUMPAD2, "Num 2"},
        {VK_NUMPAD3, "Num 3"}, {VK_NUMPAD4, "Num 4"}, {VK_NUMPAD5, "Num 5"}, {VK_NUMPAD6, "Num 6"},
        {VK_NUMPAD7, "Num 7"}, {VK_NUMPAD8, "Num 8"}, {VK_NUMPAD9, "Num 9"}, {VK_MULTIPLY, "Num *"},
        {VK_ADD, "Num +"}, {VK_SEPARATOR, "Num Sep"}, {VK_SUBTRACT, "Num -"}, {VK_DECIMAL, "Num ."},
        {VK_DIVIDE, "Num /"}, {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"},
        {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"}, {VK_F9, "F9"},
        {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"}, {VK_NUMLOCK, "Num Lock"},
        {VK_SCROLL, "Scroll Lock"}, {VK_OEM_1, ";"}, {VK_OEM_PLUS, "="}, {VK_OEM_COMMA, ","},
        {VK_OEM_MINUS, "-"}, {VK_OEM_PERIOD, "."}, {VK_OEM_2, "/"}, {VK_OEM_3, "`"},
        {VK_OEM_4, "["}, {VK_OEM_5, "\\"}, {VK_OEM_6, "]"}, {VK_OEM_7, "'"},
    };
    auto it = keyNames.find(vk_code);
    if (it != keyNames.end()) return it->second;
    static char unknownKey[32];
    snprintf(unknownKey, sizeof(unknownKey), "VK 0x%02X", vk_code);
    return unknownKey;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
HackGUI::HackGUI(Hack& hack) : m_hack(hack) {
    // Initialize hotkeys with defaults
    m_key_savepos = Constants::Hotkeys::KEY_SAVEPOS;
    m_key_loadpos = Constants::Hotkeys::KEY_LOADPOS;
    m_key_invisibility = Constants::Hotkeys::KEY_INVISIBILITY;
    m_key_wallclimb = Constants::Hotkeys::KEY_WALLCLIMB;
    m_key_clipping = Constants::Hotkeys::KEY_CLIPPING;
    m_key_object_clipping = Constants::Hotkeys::KEY_OBJECT_CLIPPING;
    m_key_full_strafe = Constants::Hotkeys::KEY_FULL_STRAFE;
    m_key_no_fog = Constants::Hotkeys::KEY_NO_FOG;
    m_key_super_sprint = Constants::Hotkeys::KEY_SUPER_SPRINT;
    m_key_sprint = Constants::Hotkeys::KEY_SPRINT;
    m_key_fly = Constants::Hotkeys::KEY_FLY;
    // TODO: Load saved hotkeys
}

//-----------------------------------------------------------------------------
// Helper: Draw single hotkey line
//-----------------------------------------------------------------------------
void HackGUI::CheckAndSetHotkey(int hotkey_index, const char* name, int& key_variable) {
    ImGui::Text("%s:", name);
    ImGui::SameLine(150.0f);
    if (m_rebinding_hotkey_index == hotkey_index) {
        ImGui::TextDisabled("<Press any key>");
    }
    else {
        ImGui::Text("%s", GetKeyName(key_variable));
        ImGui::SameLine(280.0f);
        std::string button_label = "Change##" + std::string(name);
        if (ImGui::Button(button_label.c_str())) {
            m_rebinding_hotkey_index = hotkey_index;
            m_rebinding_hotkey_name = name;
        }
    }
}

//-----------------------------------------------------------------------------
// Render "Always on Top" checkbox & logic
//-----------------------------------------------------------------------------
void HackGUI::RenderAlwaysOnTop() {
    static bool always_on_top_checkbox = false;
    static bool current_window_is_topmost = false;
    HWND current_window_hwnd = nullptr;
    ImGuiWindow* current_imgui_win = ImGui::GetCurrentWindowRead();
    if (current_imgui_win && current_imgui_win->Viewport) {
        current_window_hwnd = (HWND)current_imgui_win->Viewport->PlatformHandleRaw;
    }
    if (ImGui::Checkbox("Always on Top", &always_on_top_checkbox)) {}
    if (current_window_hwnd) {
        if (always_on_top_checkbox) {
            if (!current_window_is_topmost) {
                ::SetWindowPos(current_window_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                current_window_is_topmost = true;
            }
        }
        else {
            if (current_window_is_topmost) {
                ::SetWindowPos(current_window_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                current_window_is_topmost = false;
            }
        }
    }
    ImGui::Separator();
    ImGui::Spacing();
}

//-----------------------------------------------------------------------------
// Handle ALL hotkey checks and actions
//-----------------------------------------------------------------------------
void HackGUI::HandleHotkeys() {
    // --- Toggle Keys (using edge detection: & 1) ---
    if (m_key_no_fog != 0 && (GetAsyncKeyState(m_key_no_fog) & 1)) {
        m_fogEnabled = !m_fogEnabled;
        m_hack.toggleFog(m_fogEnabled);
    }
    if (m_key_object_clipping != 0 && (GetAsyncKeyState(m_key_object_clipping) & 1)) {
        m_objectClippingEnabled = !m_objectClippingEnabled;
        m_hack.toggleObjectClipping(m_objectClippingEnabled);
    }
    if (m_key_full_strafe != 0 && (GetAsyncKeyState(m_key_full_strafe) & 1)) {
        m_fullStrafeEnabled = !m_fullStrafeEnabled;
        m_hack.toggleFullStrafe(m_fullStrafeEnabled);
    }
    if (m_key_invisibility != 0 && (GetAsyncKeyState(m_key_invisibility) & 1)) {
        m_invisibilityEnabled = !m_invisibilityEnabled;
        m_hack.toggleInvisibility(m_invisibilityEnabled);
    }
    if (m_key_wallclimb != 0 && (GetAsyncKeyState(m_key_wallclimb) & 1)) {
        m_wallClimbEnabled = !m_wallClimbEnabled;
        m_hack.toggleWallClimb(m_wallClimbEnabled);
    }
    if (m_key_clipping != 0 && (GetAsyncKeyState(m_key_clipping) & 1)) {
        m_clippingEnabled = !m_clippingEnabled;
        m_hack.toggleClipping(m_clippingEnabled);
    }

    // --- Sprint Toggle (special case based on its checkbox) ---
    if (m_sprintEnabled && m_key_sprint != 0) { // Check master enable and if key is bound
        if (GetAsyncKeyState(m_key_sprint) & 1) {
            m_sprintActive = !m_sprintActive; // Toggle the internal state
            // No direct call to m_hack here, applied below
        }
    }
    // Apply sprint state every frame based on the toggled state and master checkbox
    m_hack.handleSprint(m_sprintEnabled && m_sprintActive && m_key_sprint != 0);

    // --- Hold Keys (checking current state: & 0x8000) ---
    // Super Sprint
    bool superSprintKeyPressed = (m_key_super_sprint != 0 && (GetAsyncKeyState(m_key_super_sprint) & 0x8000) != 0);
    if (superSprintKeyPressed != m_superSprintActive) { // Check if state *changed*
        m_superSprintActive = superSprintKeyPressed;
        m_hack.handleSuperSprint(m_superSprintActive); // Notify Hack only on change
    }
    // Fly
    bool flyKeyPressed = (m_key_fly != 0 && (GetAsyncKeyState(m_key_fly) & 0x8000) != 0);
    if (flyKeyPressed != m_flyActive) { // Check if state *changed*
        m_flyActive = flyKeyPressed;
        m_hack.handleFly(m_flyActive); // Notify Hack only on change
    }

    // --- Action Keys (using edge detection: & 1) ---
    if (m_key_savepos != 0 && (GetAsyncKeyState(m_key_savepos) & 1)) {
        m_hack.savePosition();
        // Optional: m_hack.reportStatus("INFO: Position Saved (Hotkey)");
    }
    if (m_key_loadpos != 0 && (GetAsyncKeyState(m_key_loadpos) & 1)) {
        m_hack.loadPosition();
        // Optional: m_hack.reportStatus("INFO: Position Loaded (Hotkey)");
    }
}

//-----------------------------------------------------------------------------
// Handle capturing new hotkey bindings
//-----------------------------------------------------------------------------
void HackGUI::HandleHotkeyRebinding() {
    if (m_rebinding_hotkey_index == -1) return; // Not rebinding

    int captured_vk = -1;

    if (GetAsyncKeyState(VK_ESCAPE) & 1) { captured_vk = VK_ESCAPE; }
    else if ((GetAsyncKeyState(VK_DELETE) & 1) || (GetAsyncKeyState(VK_BACK) & 1)) { captured_vk = 0; }
    else {
        for (int vk = VK_MBUTTON; vk < VK_OEM_CLEAR; ++vk) {
            if (// Special action keys for rebinding
                vk == VK_ESCAPE || vk == VK_DELETE || vk == VK_BACK ||
                // Mouse buttons that interfere with UI clicking
                vk == VK_LBUTTON || vk == VK_RBUTTON ||
                // Generic modifiers (ambiguous, bind L/R specific instead)
                vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU ||
                // Toggle keys that are usually not intended for direct binding
                vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL
                )
            {
                continue; // Skip this key
            }

            if (GetAsyncKeyState(vk) & 1) { captured_vk = vk; break; }
        }
    }w

    // If action was taken
    if (captured_vk != -1) {
        if (captured_vk != VK_ESCAPE) {
            int* key_to_set = nullptr;
            switch (m_rebinding_hotkey_index) {
            case 0: key_to_set = &m_key_savepos; break; case 1: key_to_set = &m_key_loadpos; break;
            case 2: key_to_set = &m_key_invisibility; break; case 3: key_to_set = &m_key_wallclimb; break;
            case 4: key_to_set = &m_key_clipping; break; case 5: key_to_set = &m_key_object_clipping; break;
            case 6: key_to_set = &m_key_full_strafe; break; case 7: key_to_set = &m_key_no_fog; break;
            case 8: key_to_set = &m_key_super_sprint; break; case 9: key_to_set = &m_key_sprint; break;
            case 10: key_to_set = &m_key_fly; break;
            }
            if (key_to_set) { *key_to_set = captured_vk; /* TODO: Save config */ }
        }
        m_rebinding_hotkey_index = -1; // Reset state
        m_rebinding_hotkey_name = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Render Toggles Section
//-----------------------------------------------------------------------------
void HackGUI::RenderTogglesSection() {
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Checkboxes now primarily reflect state, but can also change it.
        // Hotkeys also change the underlying m_XXXEnabled state.
        if (ImGui::Checkbox("No Fog", &m_fogEnabled)) { m_hack.toggleFog(m_fogEnabled); }
        if (ImGui::Checkbox("Object Clipping", &m_objectClippingEnabled)) { m_hack.toggleObjectClipping(m_objectClippingEnabled); }
        if (ImGui::Checkbox("Full Strafe", &m_fullStrafeEnabled)) { m_hack.toggleFullStrafe(m_fullStrafeEnabled); }
        if (ImGui::Checkbox("Enable Sprint Key", &m_sprintEnabled)) {
            if (!m_sprintEnabled) { m_sprintActive = false; } // Force off if disabled
        }
        ImGui::SameLine(); ImGui::TextDisabled("(Toggles with %s)", GetKeyName(m_key_sprint));
        if (ImGui::Checkbox("Invisibility (Mobs)", &m_invisibilityEnabled)) { m_hack.toggleInvisibility(m_invisibilityEnabled); }
        if (ImGui::Checkbox("Wall Climb", &m_wallClimbEnabled)) { m_hack.toggleWallClimb(m_wallClimbEnabled); }
        if (ImGui::Checkbox("Clipping", &m_clippingEnabled)) { m_hack.toggleClipping(m_clippingEnabled); }
        ImGui::Spacing();
    }
}

//-----------------------------------------------------------------------------
// Render Actions Section
//-----------------------------------------------------------------------------
void HackGUI::RenderActionsSection() {
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        float button_width = ImGui::GetContentRegionAvail().x * 0.48f;
        if (ImGui::Button("Save Position", ImVec2(button_width, 0))) { m_hack.savePosition(); }
        ImGui::SameLine();
        if (ImGui::Button("Load Position", ImVec2(-1.0f, 0))) { m_hack.loadPosition(); }
        ImGui::Spacing();
    }
}

//-----------------------------------------------------------------------------
// Render Keys Status Section (REMOVED - No longer needed)
//-----------------------------------------------------------------------------
// void HackGUI::RenderKeysStatusSection() { /* ... */ } // DELETE THIS FUNCTION

//-----------------------------------------------------------------------------
// Render Hotkeys Section
//-----------------------------------------------------------------------------
void HackGUI::RenderHotkeysSection() {
    if (ImGui::CollapsingHeader("Hotkeys")) {
        if (m_rebinding_hotkey_index != -1) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rebinding '%s'. Press a key (ESC to cancel, DEL/BKSP to clear)...", m_rebinding_hotkey_name);
            ImGui::Separator();
        }

        bool flags_pushed = false;
        if (m_rebinding_hotkey_index != -1) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            flags_pushed = true;
        }

        // Action Hotkeys
        CheckAndSetHotkey(0, "Save Position", m_key_savepos);
        CheckAndSetHotkey(1, "Load Position", m_key_loadpos);
        ImGui::Separator(); // Separate action keys from toggles
        // Toggle Hotkeys
        CheckAndSetHotkey(7, "No Fog", m_key_no_fog);
        CheckAndSetHotkey(5, "Object Clipping", m_key_object_clipping);
        CheckAndSetHotkey(6, "Full Strafe", m_key_full_strafe);
        CheckAndSetHotkey(2, "Invisibility", m_key_invisibility);
        CheckAndSetHotkey(3, "Wall Climb", m_key_wallclimb);
        CheckAndSetHotkey(4, "Clipping", m_key_clipping);
        CheckAndSetHotkey(9, "Sprint", m_key_sprint);
        ImGui::Separator(); // Separate hold keys
        // Hold Hotkeys
        CheckAndSetHotkey(8, "Super Sprint", m_key_super_sprint);
        CheckAndSetHotkey(10, "Fly", m_key_fly);


        if (flags_pushed) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::Spacing();
    }
}

//-----------------------------------------------------------------------------
// Render Log Section
//-----------------------------------------------------------------------------
void HackGUI::RenderLogSection() {
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_None)) {
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar); {
            std::lock_guard<std::mutex> lock(g_statusMutex);
            for (const auto& msg : g_statusMessages) {
                ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                if (msg.rfind("ERROR:", 0) == 0) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                else if (msg.rfind("WARN:", 0) == 0) color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
                else if (msg.rfind("INFO:", 0) == 0) color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                ImGui::TextColored(color, "%s", msg.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2) ImGui::SetScrollHereY(1.0f);
        } ImGui::EndChild();
        if (ImGui::Button("Clear Log")) { std::lock_guard<std::mutex> lock(g_statusMutex); g_statusMessages.clear(); }
        ImGui::Spacing();
    }
}

//-----------------------------------------------------------------------------
// Render Info Section (Simplified)
//-----------------------------------------------------------------------------
void HackGUI::RenderInfoSection() {
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        // Key reminders removed - the Hotkeys section is the source of truth
        ImGui::Text("Consider the paid version at kxtools.xyz!");
        ImGui::Spacing();
    }
}

//-----------------------------------------------------------------------------
// Main UI rendering function
//-----------------------------------------------------------------------------
bool HackGUI::renderUI()
{
    static bool main_window_open = true;
    bool exit_requested = false;

    ImGuiWindowFlags window_flags = 0;
    ImGui::Begin("KX Trainer", &main_window_open, window_flags);

    if (!main_window_open) { exit_requested = true; }

    // Render Top Controls
    RenderAlwaysOnTop();

    // Update Core Logic States
    m_hack.refreshAddresses();
    HandleHotkeys(); // Handles ALL hotkey checks and applies state
    HandleHotkeyRebinding(); // Handles input capture if rebinding

    // Render UI Sections
    RenderTogglesSection();    // Checkboxes reflect state changed by hotkeys too
    RenderActionsSection();
    // RenderKeysStatusSection(); // REMOVED
    RenderHotkeysSection();      // Definitive list of current bindings
    RenderLogSection();
    RenderInfoSection();       // Simplified, no key reminders

    ImGui::End(); // End main window

    return exit_requested;
}