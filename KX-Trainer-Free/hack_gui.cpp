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

const char* HackGUI::GetKeyName(int vk_code) {
    if (vk_code == 0) { return "None"; }
    // Static map for efficient lookup, initialized once
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

    // Fallback for unknown keys
    static char unknownKey[32];
    snprintf(unknownKey, sizeof(unknownKey), "VK 0x%02X", vk_code);
    return unknownKey;
}

HackGUI::HackGUI(Hack& hack) : m_hack(hack) {
    // Initialize hotkeys to unbound
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

void HackGUI::CheckAndSetHotkey(int hotkey_index, const char* name, int& key_variable) {
    ImGui::Text("%s:", name);
    ImGui::SameLine(150.0f); // Alignment for key name/status
    if (m_rebinding_hotkey_index == hotkey_index) {
        ImGui::TextDisabled("<Press any key>");
    }
    else {
        ImGui::Text("%s", GetKeyName(key_variable));
        ImGui::SameLine(280.0f); // Alignment for change button
        std::string button_label = "Change##" + std::string(name); // Unique ID for button
        if (ImGui::Button(button_label.c_str())) {
            m_rebinding_hotkey_index = hotkey_index;
            m_rebinding_hotkey_name = name;
        }
    }
}

void HackGUI::RenderAlwaysOnTop() {
    static bool always_on_top_checkbox = false;
    static bool current_window_is_topmost = false; // Track OS window state
    HWND current_window_hwnd = nullptr;

    // Get the HWND associated with the current ImGui window
    ImGuiWindow* current_imgui_win = ImGui::GetCurrentWindowRead();
    if (current_imgui_win && current_imgui_win->Viewport) {
        current_window_hwnd = (HWND)current_imgui_win->Viewport->PlatformHandleRaw;
    }

    ImGui::Checkbox("Always on Top", &always_on_top_checkbox);

    // Apply the topmost status if the HWND is valid
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

void HackGUI::HandleHotkeys() {
    // Toggles (on key press)
    if (m_key_no_fog != 0 && (GetAsyncKeyState(m_key_no_fog) & 1)) {
        m_fogEnabled = !m_fogEnabled; m_hack.toggleFog(m_fogEnabled);
    }
    if (m_key_object_clipping != 0 && (GetAsyncKeyState(m_key_object_clipping) & 1)) {
        m_objectClippingEnabled = !m_objectClippingEnabled; m_hack.toggleObjectClipping(m_objectClippingEnabled);
    }
    if (m_key_full_strafe != 0 && (GetAsyncKeyState(m_key_full_strafe) & 1)) {
        m_fullStrafeEnabled = !m_fullStrafeEnabled; m_hack.toggleFullStrafe(m_fullStrafeEnabled);
    }
    if (m_key_invisibility != 0 && (GetAsyncKeyState(m_key_invisibility) & 1)) {
        m_invisibilityEnabled = !m_invisibilityEnabled; m_hack.toggleInvisibility(m_invisibilityEnabled);
    }
    if (m_key_wallclimb != 0 && (GetAsyncKeyState(m_key_wallclimb) & 1)) {
        m_wallClimbEnabled = !m_wallClimbEnabled; m_hack.toggleWallClimb(m_wallClimbEnabled);
    }
    if (m_key_clipping != 0 && (GetAsyncKeyState(m_key_clipping) & 1)) {
        m_clippingEnabled = !m_clippingEnabled; m_hack.toggleClipping(m_clippingEnabled);
    }
    if (m_key_sprint != 0 && (GetAsyncKeyState(m_key_sprint) & 1)) {
        m_sprintEnabled = !m_sprintEnabled;
    }

    // Holds (while key is down)
    bool superSprintKeyPressed = (m_key_super_sprint != 0 && (GetAsyncKeyState(m_key_super_sprint) & 0x8000) != 0);
    if (superSprintKeyPressed != m_superSprintActive) {
        m_superSprintActive = superSprintKeyPressed; m_hack.handleSuperSprint(m_superSprintActive);
    }
    bool flyKeyPressed = (m_key_fly != 0 && (GetAsyncKeyState(m_key_fly) & 0x8000) != 0);
    if (flyKeyPressed != m_flyActive) {
        m_flyActive = flyKeyPressed; m_hack.handleFly(m_flyActive);
    }

    // Actions (on key press)
    if (m_key_savepos != 0 && (GetAsyncKeyState(m_key_savepos) & 1)) { m_hack.savePosition(); }
    if (m_key_loadpos != 0 && (GetAsyncKeyState(m_key_loadpos) & 1)) { m_hack.loadPosition(); }
}

void HackGUI::HandleHotkeyRebinding() {
    if (m_rebinding_hotkey_index == -1) return;

    int captured_vk = -1; // Use -1 to indicate no key captured yet

    // Check special keys first
    if (GetAsyncKeyState(VK_ESCAPE) & 1) { captured_vk = VK_ESCAPE; } // Cancel
    else if ((GetAsyncKeyState(VK_DELETE) & 1) || (GetAsyncKeyState(VK_BACK) & 1)) { captured_vk = 0; } // Unbind (set to None)
    else {
        // Iterate through relevant key codes
        for (int vk = VK_MBUTTON; vk < VK_OEM_CLEAR; ++vk) {
            // Skip keys that interfere with rebinding or are generally unsuitable
            if (vk == VK_ESCAPE || vk == VK_DELETE || vk == VK_BACK ||
                vk == VK_LBUTTON || vk == VK_RBUTTON || // Avoid capturing UI clicks
                vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || // Prefer specific L/R versions
                vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL // State keys
                )
            {
                continue;
            }

            if (GetAsyncKeyState(vk) & 1) { // Check for key press event
                captured_vk = vk;
                break;
            }
        }
    }

    // If a key was captured or an action (ESC/DEL/BKSP) was taken
    if (captured_vk != -1) {
        if (captured_vk != VK_ESCAPE) { // Don't assign if ESC was pressed
            int* key_to_set = nullptr;
            // Map index back to the correct member variable
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
                // default: should not happen
            }
            if (key_to_set) {
                *key_to_set = captured_vk;
                // TODO: Save updated hotkeys to config file
            }
        }
        // Reset rebinding state regardless of whether a key was assigned
        m_rebinding_hotkey_index = -1;
        m_rebinding_hotkey_name = nullptr;
    }
}

void HackGUI::RenderTogglesSection() {
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("No Fog", &m_fogEnabled)) { m_hack.toggleFog(m_fogEnabled); }
        if (ImGui::Checkbox("Object Clipping", &m_objectClippingEnabled)) { m_hack.toggleObjectClipping(m_objectClippingEnabled); }
        if (ImGui::Checkbox("Full Strafe", &m_fullStrafeEnabled)) { m_hack.toggleFullStrafe(m_fullStrafeEnabled); }
        ImGui::Checkbox("Sprint", &m_sprintEnabled); // Toggles user preference; actual sprint applied elsewhere
        if (ImGui::Checkbox("Invisibility (Mobs)", &m_invisibilityEnabled)) { m_hack.toggleInvisibility(m_invisibilityEnabled); }
        if (ImGui::Checkbox("Wall Climb", &m_wallClimbEnabled)) { m_hack.toggleWallClimb(m_wallClimbEnabled); }
        if (ImGui::Checkbox("Clipping", &m_clippingEnabled)) { m_hack.toggleClipping(m_clippingEnabled); }
        ImGui::Spacing();
    }
}

void HackGUI::RenderActionsSection() {
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        float button_width = ImGui::GetContentRegionAvail().x * 0.48f; // Roughly half width
        if (ImGui::Button("Save Position", ImVec2(button_width, 0))) { m_hack.savePosition(); }
        ImGui::SameLine();
        if (ImGui::Button("Load Position", ImVec2(-1.0f, 0))) { m_hack.loadPosition(); } // Fill remaining width
        ImGui::Spacing();
    }
}

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
        CheckAndSetHotkey(9, "Sprint", m_key_sprint);
        ImGui::Separator();
        CheckAndSetHotkey(8, "Super Sprint", m_key_super_sprint); // Hold
        CheckAndSetHotkey(10, "Fly", m_key_fly);                   // Hold

        if (flags_pushed) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Default and Unbind buttons
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

void HackGUI::RenderLogSection() {
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_None)) { // Start collapsed by default
        // Scrolling region for log messages
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            // Get a thread-safe copy of the messages from StatusUI
            std::vector<std::string> current_messages = StatusUI::GetMessages(); // <--- USE GetMessages()

            // No need to lock mutex here, we are working with a copy
            for (const auto& msg : current_messages) { // <--- Iterate the copy
                // Basic coloring based on prefix
                ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text); // Default
                if (msg.rfind("ERROR:", 0) == 0) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
                else if (msg.rfind("WARN:", 0) == 0) color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow
                else if (msg.rfind("INFO:", 0) == 0) color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green
                ImGui::TextColored(color, "%s", msg.c_str());
            }
            // Auto-scroll to bottom if near the end
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Clear Log")) {
            StatusUI::ClearMessages(); // <--- USE ClearMessages()
        }
        ImGui::Spacing();
    }
}

void HackGUI::RenderInfoSection() {
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        ImGui::Text("Consider the paid version at kxtools.xyz!");
        ImGui::Spacing();
    }
}

bool HackGUI::renderUI()
{
    static bool main_window_open = true;
    bool exit_requested = false;

    // Set minimum window size constraints
    const float min_window_width = 400.0f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(min_window_width, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    ImGuiWindowFlags window_flags = 0;
    // Setting ImGuiCond_FirstUseEver for initial size might be useful here too
    // ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("KX Trainer", &main_window_open, window_flags);

    if (!main_window_open) {
        exit_requested = true; // Request exit if user closes the window
    }

    // Top-level controls
    RenderAlwaysOnTop();

    // Update game state and handle input
    m_hack.refreshAddresses(); // Ensure pointers are valid
    HandleHotkeys();           // Check and act on bound keys
    HandleHotkeyRebinding();   // Handle input if currently rebinding a key

    // Apply continuous states
    m_hack.handleSprint(m_sprintEnabled);

    // Render main UI sections
    RenderTogglesSection();
    RenderActionsSection();
    RenderHotkeysSection();
    RenderLogSection();
    RenderInfoSection();

    ImGui::End();

    return exit_requested;
}