#include "hack_gui.h"
#include "hack.h"
#include "constants.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>

extern std::vector<std::string> g_statusMessages;
extern std::mutex g_statusMutex;

using namespace Constants::Hotkeys;

HackGUI::HackGUI(Hack& hack) : m_hack(hack) {
    // TODO: Optionally pre-read initial toggle states from m_hack here
}

bool HackGUI::renderUI()
{
    static bool main_window_open = true;
    static bool always_on_top_checkbox = false;
    static bool current_window_is_topmost = false;

    bool exit_requested = false;

    ImGuiWindowFlags window_flags = 0;
    ImGui::Begin("KX Trainer", &main_window_open, window_flags);

    if (!main_window_open) {
        exit_requested = true;
    }

    // --- Always On Top Feature ---
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


    m_hack.refreshAddresses();


    // --- Handle Sprint Toggle Logic (Simplified) ---
    if (m_sprintEnabled) { // Only process toggle if the feature is enabled via checkbox
        // Check if the sprint key was pressed *since the last call* (edge detection)
        if (GetAsyncKeyState(KEY_SPRINT) & 1) {
            m_sprintActive = !m_sprintActive; // Toggle the state
            // Optional: Add a status message via callback if desired
            // m_hack.reportStatus("INFO: Sprint Toggled " + std::string(m_sprintActive ? "ON" : "OFF"));
        }
    }
    // Call the hack's sprint handler every frame with the current *toggled* state
    // Ensure sprint is off if the master enable checkbox is off.
    m_hack.handleSprint(m_sprintEnabled && m_sprintActive);
    // --- End Sprint Toggle Logic ---


    // -- Toggles Section --
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("No Fog", &m_fogEnabled)) { m_hack.toggleFog(m_fogEnabled); }
        if (ImGui::Checkbox("Object Clipping", &m_objectClippingEnabled)) { m_hack.toggleObjectClipping(m_objectClippingEnabled); }
        if (ImGui::Checkbox("Full Strafe", &m_fullStrafeEnabled)) { m_hack.toggleFullStrafe(m_fullStrafeEnabled); }

        // Sprint Enable Checkbox: Allows the sprint key (Shift) to toggle the sprint state
        if (ImGui::Checkbox("Enable Sprint Key", &m_sprintEnabled)) {
            // If the master toggle is disabled, ensure the active state reflects this visually
            // The actual call to m_hack.handleSprint above already handles forcing it off.
            if (!m_sprintEnabled) {
                m_sprintActive = false; // Ensure visual consistency if disabled while active
            }
        }
        ImGui::SameLine(); ImGui::TextDisabled("(Toggles with Shift)");

        if (ImGui::Checkbox("Invisibility (Mobs)", &m_invisibilityEnabled)) { m_hack.toggleInvisibility(m_invisibilityEnabled); }
        if (ImGui::Checkbox("Wall Climb", &m_wallClimbEnabled)) { m_hack.toggleWallClimb(m_wallClimbEnabled); }
        if (ImGui::Checkbox("Clipping", &m_clippingEnabled)) { m_hack.toggleClipping(m_clippingEnabled); }
        ImGui::Spacing();
    }

    // -- Actions Section --
    // (Code remains the same)
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        float button_width = ImGui::GetContentRegionAvail().x * 0.48f;
        if (ImGui::Button("Save Position", ImVec2(button_width, 0))) { m_hack.savePosition(); }
        ImGui::SameLine();
        if (ImGui::Button("Load Position", ImVec2(-1.0f, 0))) { m_hack.loadPosition(); }
        ImGui::Spacing();
    }

    // -- Keys Status Section --
    if (ImGui::CollapsingHeader("Keys Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Check Super Sprint key (Hold)
        bool superSprintKeyPressed = (GetAsyncKeyState(KEY_SUPER_SPRINT) & 0x8000) != 0;
        // Only call handler if state changes for hold keys
        if (superSprintKeyPressed != m_superSprintEnabled) {
            m_superSprintEnabled = superSprintKeyPressed;
            m_hack.handleSuperSprint(m_superSprintEnabled);
        }
        ImGui::Text("Super Sprint (%s): %s", "Num+", m_superSprintEnabled ? "Active (Holding)" : "Inactive");

        // Check Fly key (Hold)
        bool flyKeyPressed = (GetAsyncKeyState(KEY_FLY) & 0x8000) != 0;
        // Only call handler if state changes for hold keys
        if (flyKeyPressed != m_flyEnabled) {
            m_flyEnabled = flyKeyPressed;
            m_hack.handleFly(m_flyEnabled);
        }
        ImGui::Text("Fly (%s):          %s", "Ctrl", m_flyEnabled ? "Active (Holding)" : "Inactive");

        // Display Sprint status (Toggle)
        // Status depends on the m_sprintActive state, which is toggled by key press
        ImGui::Text("Sprint (%s):       %s", "Shift", m_sprintActive ? "Active (Toggled)" : "Inactive");
        if (!m_sprintEnabled) { // Show hint if master toggle is off
            ImGui::SameLine(); ImGui::TextDisabled("(Enable checkbox first)");
        }
        ImGui::Spacing();
    }

    // -- Log Section --
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_None)) {
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lock(g_statusMutex);
            for (const auto& msg : g_statusMessages) {
                ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                if (msg.rfind("ERROR:", 0) == 0)      color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                else if (msg.rfind("WARN:", 0) == 0)  color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
                else if (msg.rfind("INFO:", 0) == 0)  color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                ImGui::TextColored(color, "%s", msg.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Clear Log")) {
            std::lock_guard<std::mutex> lock(g_statusMutex);
            g_statusMessages.clear();
        }
        ImGui::Spacing();
    }

    // -- Info Section --
    // (Code remains the same)
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        ImGui::Text("Key Reminders:");
        ImGui::BulletText("Super Sprint: NUMPAD+ (Hold)");
        ImGui::BulletText("Fly: Left CTRL (Hold)");
        ImGui::BulletText("Sprint: Left SHIFT (Toggle, if enabled)");
        ImGui::Separator();
        ImGui::Text("Consider the paid version at kxtools.xyz!");
        ImGui::Spacing();
    }

    ImGui::End(); // End main window

    return exit_requested;
}