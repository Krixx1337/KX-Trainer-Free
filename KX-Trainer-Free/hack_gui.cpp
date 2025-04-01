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

#include "constants.h"

HackGUI::HackGUI(Hack& hack) : m_hack(hack), m_rebinding_hotkey_id(HotkeyID::NONE) {
    // Define all available hotkeys and their default properties
    m_hotkeys = {
        {HotkeyID::SAVE_POS,             "Save Position",   Constants::Hotkeys::KEY_SAVEPOS,         HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.savePosition(); }},
        {HotkeyID::LOAD_POS,             "Load Position",   Constants::Hotkeys::KEY_LOADPOS,         HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.loadPosition(); }},
        {HotkeyID::TOGGLE_INVISIBILITY,  "Invisibility",    Constants::Hotkeys::KEY_INVISIBILITY,    HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.toggleInvisibility(!h.IsInvisibilityEnabled()); }},
        {HotkeyID::TOGGLE_WALLCLIMB,     "Wall Climb",      Constants::Hotkeys::KEY_WALLCLIMB,       HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.toggleWallClimb(!h.IsWallClimbEnabled()); }},
        {HotkeyID::TOGGLE_CLIPPING,      "Clipping",        Constants::Hotkeys::KEY_CLIPPING,        HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.toggleClipping(!h.IsClippingEnabled()); }},
        {HotkeyID::TOGGLE_OBJECT_CLIPPING,"Object Clipping", Constants::Hotkeys::KEY_OBJECT_CLIPPING, HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.toggleObjectClipping(!h.IsObjectClippingEnabled()); }},
        {HotkeyID::TOGGLE_FULL_STRAFE,   "Full Strafe",     Constants::Hotkeys::KEY_FULL_STRAFE,     HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.toggleFullStrafe(!h.IsFullStrafeEnabled()); }},
        {HotkeyID::TOGGLE_NO_FOG,        "No Fog",          Constants::Hotkeys::KEY_NO_FOG,          HotkeyTriggerType::ON_PRESS, [](Hack& h, bool) { h.toggleFog(!h.IsFogEnabled()); }},
        {HotkeyID::HOLD_SUPER_SPRINT,    "Super Sprint",    Constants::Hotkeys::KEY_SUPER_SPRINT,    HotkeyTriggerType::ON_HOLD,  [](Hack& h, bool held) { h.handleSuperSprint(held); }},
        {HotkeyID::TOGGLE_SPRINT_PREF,   "Sprint",          Constants::Hotkeys::KEY_SPRINT,          HotkeyTriggerType::ON_PRESS, [this](Hack& /*h*/, bool) { this->m_sprintEnabled = !this->m_sprintEnabled; }}, // Toggles the GUI preference flag
        {HotkeyID::HOLD_FLY,             "Fly",             Constants::Hotkeys::KEY_FLY,             HotkeyTriggerType::ON_HOLD,  [](Hack& h, bool held) { h.handleFly(held); }}
    };

    // TODO: Load saved currentKeyCode values from a config file here, overwriting the defaults set in HotkeyInfo constructor
}

// Renders label, key name, and Change button for a single hotkey configuration
void HackGUI::RenderHotkeyControl(HotkeyInfo& hotkey) {
    ImGui::Text("%s:", hotkey.name);
    ImGui::SameLine(150.0f); // Alignment

    if (m_rebinding_hotkey_id == hotkey.id) {
        ImGui::TextDisabled("<Press any key>");
    } else {
        ImGui::Text("%s", GetKeyName(hotkey.currentKeyCode)); // Use GetKeyName with the current code
        ImGui::SameLine(280.0f); // Alignment

        // Create a unique ID for the button using the hotkey name
        std::string button_label = "Change##" + std::string(hotkey.name);
        if (ImGui::Button(button_label.c_str())) {
            m_rebinding_hotkey_id = hotkey.id; // Set the ID of the hotkey being rebound
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

// Checks registered hotkeys and calls corresponding actions
void HackGUI::HandleHotkeys() {
    // Don't process hotkeys if currently rebinding one
    if (m_rebinding_hotkey_id != HotkeyID::NONE) {
        return;
    }

    for (const auto& hotkey : m_hotkeys) {
        if (hotkey.currentKeyCode == 0 || !hotkey.action) { // Skip unbound or unassigned actions
            continue;
        }

        SHORT keyState = GetAsyncKeyState(hotkey.currentKeyCode);

        switch (hotkey.triggerType) {
            case HotkeyTriggerType::ON_PRESS:
                // Check the least significant bit (1 means pressed *since the last call*)
                if (keyState & 1) {
                    hotkey.action(m_hack, true); // Pass true for pressed state
                }
                break;

            case HotkeyTriggerType::ON_HOLD:
                // Check the most significant bit (1 means currently held down)
                bool isHeld = (keyState & 0x8000) != 0;
                hotkey.action(m_hack, isHeld); // Pass the current hold state
                break;
        }
    }
}

// Handles the logic when the user is actively rebinding a hotkey
void HackGUI::HandleHotkeyRebinding() {
    if (m_rebinding_hotkey_id == HotkeyID::NONE) return;

    int captured_vk = -1; // -1 means no key captured yet

    // Check special keys first
    if (GetAsyncKeyState(VK_ESCAPE) & 1) {
        captured_vk = VK_ESCAPE; // Special value to indicate cancellation
    } else if ((GetAsyncKeyState(VK_DELETE) & 1) || (GetAsyncKeyState(VK_BACK) & 1)) {
        captured_vk = 0; // 0 means unbind
    } else {
        // Iterate through common key codes to find the first pressed key
        for (int vk = VK_MBUTTON; vk < VK_OEM_CLEAR; ++vk) {
            // Skip keys that interfere, are unsuitable, or handled above
            if (vk == VK_ESCAPE || vk == VK_DELETE || vk == VK_BACK ||
                vk == VK_LBUTTON || vk == VK_RBUTTON || // Avoid UI clicks binding easily
                vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || // Prefer specific L/R versions if needed, though GetKeyName handles these
                vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL) // State keys are poor choices
            {
                continue;
            }
            // Check if key was pressed *since the last call*
            if (GetAsyncKeyState(vk) & 1) {
                captured_vk = vk;
                break; // Found the first pressed key
            }
        }
    }

    // If a key was captured or an action key (Esc/Del/Back) was pressed
    if (captured_vk != -1) {
        if (captured_vk != VK_ESCAPE) { // If not cancelling
            // Find the hotkey being rebound in the vector
            for (auto& hotkey : m_hotkeys) {
                if (hotkey.id == m_rebinding_hotkey_id) {
                    hotkey.currentKeyCode = captured_vk; // Assign the captured key (or 0 for unbind)
                    // TODO: Save updated hotkeys to config file here
                    break; // Found and updated
                }
            }
        }
        // Reset rebinding state regardless of whether we assigned or cancelled
        m_rebinding_hotkey_id = HotkeyID::NONE;
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
        // Display rebinding prompt if active
        if (m_rebinding_hotkey_id != HotkeyID::NONE) {
            const char* rebinding_name = "Unknown"; // Fallback
            for(const auto& hk : m_hotkeys) {
                if (hk.id == m_rebinding_hotkey_id) {
                    rebinding_name = hk.name;
                    break;
                }
            }
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rebinding '%s'. Press a key (ESC to cancel, DEL/BKSP to clear)...", rebinding_name);
            ImGui::Separator();
        }

        // Dim controls while rebinding
        bool disable_controls = (m_rebinding_hotkey_id != HotkeyID::NONE);
        if (disable_controls) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        // Iterate through hotkeys and render controls
        for (auto& hotkey : m_hotkeys) {
            RenderHotkeyControl(hotkey);
        }


        if (disable_controls) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::Separator();
        ImGui::Spacing();

        // Buttons for defaults and unbinding
        if (ImGui::Button("Apply Recommended Defaults")) {
            for (auto& hotkey : m_hotkeys) {
                hotkey.currentKeyCode = hotkey.defaultKeyCode;
            }
            // TODO: Save updated hotkeys to config file
        }
        ImGui::SameLine();
        if (ImGui::Button("Unbind All")) {
            for (auto& hotkey : m_hotkeys) {
                hotkey.currentKeyCode = 0; // 0 represents unbound
            }
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

    m_hack.refreshAddresses(); // Ensure pointers are valid before reading/writing
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