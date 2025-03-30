#include "hack_gui.h"
#include "hack.h"
#include "constants.h"
#include "status_ui.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <cstdio> // For snprintf

// Helper to get human-readable key names
const char* HackGUI::GetKeyName(int vk_code) {
    if (vk_code == 0) { return "None"; }
    // Static map for efficiency
    static std::map<int, const char*> keyNames = {
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

HackGUI::HackGUI(Hack& hack) : m_hack(hack) {
    // Initialize hotkeys (or load from config)
    m_key_savepos = 0;
    m_key_loadpos = 0;
    m_key_invisibility = 0;
    m_key_wallclimb = 0;
    m_key_clipping = 0;
    m_key_object_clipping = 0;
    m_key_full_strafe = 0;
    m_key_no_fog = 0;
    m_key_super_sprint = 0;
    m_key_sprint = 0;
    m_key_fly = 0;

    // TODO: Load saved hotkeys from a config file here
}

// Renders label, key name, and Change button for a hotkey setting
void HackGUI::CheckAndSetHotkey(int hotkey_index, const char* name, int& key_variable) {
    ImGui::Text("%s:", name);
    ImGui::SameLine(150.0f); // Alignment
    if (m_rebinding_hotkey_index == hotkey_index) {
        ImGui::TextDisabled("<Press any key>");
    }
    else {
        ImGui::Text("%s", GetKeyName(key_variable));
        ImGui::SameLine(280.0f); // Alignment
        std::string button_label = "Change##" + std::string(name); // Unique ID
        if (ImGui::Button(button_label.c_str())) {
            m_rebinding_hotkey_index = hotkey_index;
            m_rebinding_hotkey_name = name;
        }
    }
}

// Renders the "Always on Top" checkbox and applies the setting
void HackGUI::RenderAlwaysOnTop() {
    static bool always_on_top_checkbox = false;
    static bool current_window_is_topmost = false;
    HWND current_window_hwnd = nullptr;

    ImGuiWindow* current_imgui_win = ImGui::GetCurrentWindowRead();
    if (current_imgui_win && current_imgui_win->Viewport) {
        current_window_hwnd = (HWND)current_imgui_win->Viewport->PlatformHandleRaw;
    }

    ImGui::Checkbox("Always on Top", &always_on_top_checkbox);

    if (current_window_hwnd) {
        HWND insert_after = always_on_top_checkbox ? HWND_TOPMOST : HWND_NOTOPMOST;
        bool should_be_topmost = always_on_top_checkbox;

        if (should_be_topmost != current_window_is_topmost) {
            ::SetWindowPos(current_window_hwnd, insert_after, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            current_window_is_topmost = should_be_topmost;
        }
    }
    ImGui::Separator();
    ImGui::Spacing();
}

// Checks registered hotkeys and calls corresponding Hack methods
void HackGUI::HandleHotkeys() {
    // Toggles (on key press - check bit 0)
    if (m_key_no_fog != 0 && (GetAsyncKeyState(m_key_no_fog) & 1)) {
        m_hack.toggleFog(!m_hack.IsFogEnabled());
    }
    if (m_key_object_clipping != 0 && (GetAsyncKeyState(m_key_object_clipping) & 1)) {
        m_hack.toggleObjectClipping(!m_hack.IsObjectClippingEnabled());
    }
    if (m_key_full_strafe != 0 && (GetAsyncKeyState(m_key_full_strafe) & 1)) {
        m_hack.toggleFullStrafe(!m_hack.IsFullStrafeEnabled());
    }
    if (m_key_invisibility != 0 && (GetAsyncKeyState(m_key_invisibility) & 1)) {
        m_hack.toggleInvisibility(!m_hack.IsInvisibilityEnabled());
    }
    if (m_key_wallclimb != 0 && (GetAsyncKeyState(m_key_wallclimb) & 1)) {
        m_hack.toggleWallClimb(!m_hack.IsWallClimbEnabled());
    }
    if (m_key_clipping != 0 && (GetAsyncKeyState(m_key_clipping) & 1)) {
        m_hack.toggleClipping(!m_hack.IsClippingEnabled());
    }
    // Sprint Preference Toggle
    if (m_key_sprint != 0 && (GetAsyncKeyState(m_key_sprint) & 1)) {
        m_sprintEnabled = !m_sprintEnabled;
    }

    // Holds (while key is down - check high bit 0x8000)
    bool superSprintKeyPressed = (m_key_super_sprint != 0 && (GetAsyncKeyState(m_key_super_sprint) & 0x8000) != 0);
    m_hack.handleSuperSprint(superSprintKeyPressed);

    bool flyKeyPressed = (m_key_fly != 0 && (GetAsyncKeyState(m_key_fly) & 0x8000) != 0);
    m_hack.handleFly(flyKeyPressed);

    // Actions (on key press - check bit 0)
    if (m_key_savepos != 0 && (GetAsyncKeyState(m_key_savepos) & 1)) { m_hack.savePosition(); }
    if (m_key_loadpos != 0 && (GetAsyncKeyState(m_key_loadpos) & 1)) { m_hack.loadPosition(); }
}

// Handles the logic when the user is actively rebinding a hotkey
void HackGUI::HandleHotkeyRebinding() {
    if (m_rebinding_hotkey_index == -1) return;

    int captured_vk = -1;

    // Check special keys first
    if (GetAsyncKeyState(VK_ESCAPE) & 1) { captured_vk = VK_ESCAPE; } // Cancel
    else if ((GetAsyncKeyState(VK_DELETE) & 1) || (GetAsyncKeyState(VK_BACK) & 1)) { captured_vk = 0; } // Unbind
    else {
        // Iterate through common key codes
        for (int vk = VK_MBUTTON; vk < VK_OEM_CLEAR; ++vk) {
            // Skip keys that interfere or are unsuitable
            if (vk == VK_ESCAPE || vk == VK_DELETE || vk == VK_BACK ||
                vk == VK_LBUTTON || vk == VK_RBUTTON || // Avoid UI clicks
                vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || // Prefer L/R versions
                vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL) // State keys
            {
                continue;
            }
            if (GetAsyncKeyState(vk) & 1) { // Key pressed
                captured_vk = vk;
                break;
            }
        }
    }

    if (captured_vk != -1) { // Key captured or action taken
        if (captured_vk != VK_ESCAPE) { // Don't assign Esc
            int* key_to_set = nullptr;
            switch (m_rebinding_hotkey_index) {
            case 0: key_to_set = &m_key_savepos; break;
            case 1: key_to_set = &m_key_loadpos; break;
            case 2: key_to_set = &m_key_invisibility; break;
            case 3: key_to_set = &m_key_wallclimb; break;
            case 4: key_to_set = &m_key_clipping; break;
            case 5: key_to_set = &m_key_object_clipping; break;
            case 6: key_to_set = &m_key_full_strafe; break;
            case 7: key_to_set = &m_key_no_fog; break;
            case 8: key_to_set = &m_key_super_sprint; break;
            case 9: key_to_set = &m_key_sprint; break;
            case 10: key_to_set = &m_key_fly; break;
            }
            if (key_to_set) {
                *key_to_set = captured_vk;
                // TODO: Save updated hotkeys to config file
            }
        }
        // Reset rebinding state
        m_rebinding_hotkey_index = -1;
        m_rebinding_hotkey_name = nullptr;
    }
}

// Renders the collapsible section with toggle checkboxes
void HackGUI::RenderTogglesSection() {
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool tempState = false; // Temporary variable for ImGui interaction

        tempState = m_hack.IsFogEnabled();
        if (ImGui::Checkbox("No Fog", &tempState)) { m_hack.toggleFog(tempState); }

        tempState = m_hack.IsObjectClippingEnabled();
        if (ImGui::Checkbox("Object Clipping", &tempState)) { m_hack.toggleObjectClipping(tempState); }

        tempState = m_hack.IsFullStrafeEnabled();
        if (ImGui::Checkbox("Full Strafe", &tempState)) { m_hack.toggleFullStrafe(tempState); }

        // Sprint checkbox controls the user preference flag
        ImGui::Checkbox("Sprint", &m_sprintEnabled);

        tempState = m_hack.IsInvisibilityEnabled();
        if (ImGui::Checkbox("Invisibility (Mobs)", &tempState)) { m_hack.toggleInvisibility(tempState); }

        tempState = m_hack.IsWallClimbEnabled();
        if (ImGui::Checkbox("Wall Climb", &tempState)) { m_hack.toggleWallClimb(tempState); }

        tempState = m_hack.IsClippingEnabled();
        if (ImGui::Checkbox("Clipping", &tempState)) { m_hack.toggleClipping(tempState); }

        ImGui::Spacing();
    }
}

// Renders the collapsible section with action buttons
void HackGUI::RenderActionsSection() {
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        float button_width = ImGui::GetContentRegionAvail().x * 0.48f; // Approx half width
        if (ImGui::Button("Save Position", ImVec2(button_width, 0))) { m_hack.savePosition(); }
        ImGui::SameLine();
        if (ImGui::Button("Load Position", ImVec2(-1.0f, 0))) { m_hack.loadPosition(); } // Fill remaining
        ImGui::Spacing();
    }
}

// Renders the collapsible section for configuring hotkeys
void HackGUI::RenderHotkeysSection() {
    if (ImGui::CollapsingHeader("Hotkeys")) {
        if (m_rebinding_hotkey_index != -1) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rebinding '%s'. Press a key (ESC to cancel, DEL/BKSP to clear)...", m_rebinding_hotkey_name);
            ImGui::Separator();
        }

        // Dim controls while rebinding
        bool flags_pushed = false;
        if (m_rebinding_hotkey_index != -1) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            flags_pushed = true;
        }

        // Hotkey assignment widgets
        CheckAndSetHotkey(0, "Save Position", m_key_savepos);
        CheckAndSetHotkey(1, "Load Position", m_key_loadpos);
        ImGui::Separator();
        CheckAndSetHotkey(7, "No Fog", m_key_no_fog);
        CheckAndSetHotkey(5, "Object Clipping", m_key_object_clipping);
        CheckAndSetHotkey(6, "Full Strafe", m_key_full_strafe);
        CheckAndSetHotkey(2, "Invisibility", m_key_invisibility);
        CheckAndSetHotkey(3, "Wall Climb", m_key_wallclimb);
        CheckAndSetHotkey(4, "Clipping", m_key_clipping);
        CheckAndSetHotkey(9, "Sprint", m_key_sprint); // Hotkey for the preference
        ImGui::Separator();
        CheckAndSetHotkey(8, "Super Sprint", m_key_super_sprint); // Hold
        CheckAndSetHotkey(10, "Fly", m_key_fly);                   // Hold

        if (flags_pushed) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Apply Recommended Defaults")) {
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
            // TODO: Save updated hotkeys to config file
        }
        ImGui::SameLine();
        if (ImGui::Button("Unbind All")) {
            m_key_savepos = 0; m_key_loadpos = 0; m_key_invisibility = 0;
            m_key_wallclimb = 0; m_key_clipping = 0; m_key_object_clipping = 0;
            m_key_full_strafe = 0; m_key_no_fog = 0; m_key_super_sprint = 0;
            m_key_sprint = 0; m_key_fly = 0;
            // TODO: Save updated hotkeys to config file
        }
        ImGui::Spacing();
    }
}

// Renders the collapsible log section
void HackGUI::RenderLogSection() {
    // Assumes StatusUI is still used for logging until Step 1 is done.
    // If Step 1 (ILogger) was done, this would pull from the logger instance.
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_None)) { // Start collapsed
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::vector<std::string> current_messages = StatusUI::GetMessages(); // <-- Keep using StatusUI for now

            for (const auto& msg : current_messages) {
                ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text); // Default
                if (msg.rfind("ERROR:", 0) == 0) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
                else if (msg.rfind("WARN:", 0) == 0) color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow
                else if (msg.rfind("INFO:", 0) == 0) color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green
                ImGui::TextColored(color, "%s", msg.c_str());
            }
            // Auto-scroll
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Clear Log")) {
            StatusUI::ClearMessages(); // <-- Keep using StatusUI for now
        }
        ImGui::Spacing();
    }
}

// Renders the collapsible info/about section
void HackGUI::RenderInfoSection() {
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        ImGui::Text("Consider the paid version at kxtools.xyz!");
        ImGui::Spacing();
    }
}

// Main render function for the HackGUI window
bool HackGUI::renderUI()
{
    static bool main_window_open = true;
    bool exit_requested = false;

    const float min_window_width = 400.0f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(min_window_width, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    ImGuiWindowFlags window_flags = 0;
    ImGui::Begin("KX Trainer", &main_window_open, window_flags);

    if (!main_window_open) {
        exit_requested = true; // Request exit if user closes window
    }

    RenderAlwaysOnTop();

    m_hack.refreshAddresses(); // Ensure pointers are valid before reading/writingw
    HandleHotkeys();           // Process registered hotkeys
    HandleHotkeyRebinding();   // Handle input if rebinding

    // Apply continuous states based on user preference toggles
    m_hack.handleSprint(m_sprintEnabled);

    // Render UI sections
    RenderTogglesSection();
    RenderActionsSection();
    RenderHotkeysSection();
    RenderLogSection();
    RenderInfoSection();

    ImGui::End();

    return exit_requested;
}