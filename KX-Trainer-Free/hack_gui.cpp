#include "hack_gui.h"
#include "hack.h"
#include "constants.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>

// External globals defined elsewhere
extern std::vector<std::string> g_statusMessages;
extern std::mutex g_statusMutex;

using namespace Constants::Hotkeys;

HackGUI::HackGUI(Hack& hack) : m_hack(hack) {
    // TODO: Optionally pre-read initial toggle states from m_hack here
    // to ensure UI matches game state on first render.
}

bool HackGUI::renderUI()
{
    static bool main_window_open = true;
    static bool always_on_top_checkbox = false;
    static bool current_window_is_topmost = false; // Track if HWND_TOPMOST is set

    bool exit_requested = false;

    ImGuiWindowFlags window_flags = 0;
    ImGui::Begin("KX Trainer", &main_window_open, window_flags);

    if (!main_window_open) { // Check ImGui window close button state
        exit_requested = true;
    }

    // --- Always On Top Feature ---
    HWND current_window_hwnd = nullptr;
    ImGuiWindow* current_imgui_win = ImGui::GetCurrentWindowRead();
    if (current_imgui_win && current_imgui_win->Viewport) {
        current_window_hwnd = (HWND)current_imgui_win->Viewport->PlatformHandleRaw;
    }

    if (ImGui::Checkbox("Always on Top", &always_on_top_checkbox)) {
        // State change handled below
    }

    // Apply or remove HWND_TOPMOST based on checkbox state
    if (current_window_hwnd) {
        if (always_on_top_checkbox) {
            if (!current_window_is_topmost) { // Avoid redundant calls
                ::SetWindowPos(current_window_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                current_window_is_topmost = true;
            }
        }
        else {
            if (current_window_is_topmost) { // Only remove if currently set
                ::SetWindowPos(current_window_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                current_window_is_topmost = false;
            }
        }
    }
    ImGui::Separator();
    ImGui::Spacing();

    // Refresh game addresses frequently for dynamic pointers
    m_hack.refreshAddresses();

    // -- Toggles Section --
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("No Fog", &m_fogEnabled)) { m_hack.toggleFog(m_fogEnabled); }
        if (ImGui::Checkbox("Object Clipping", &m_objectClippingEnabled)) { m_hack.toggleObjectClipping(m_objectClippingEnabled); }
        if (ImGui::Checkbox("Full Strafe", &m_fullStrafeEnabled)) { m_hack.toggleFullStrafe(m_fullStrafeEnabled); }
        ImGui::Checkbox("Enable Sprint Key", &m_sprintEnabled); ImGui::SameLine(); ImGui::TextDisabled("(Requires Shift)");
        if (ImGui::Checkbox("Invisibility (Mobs)", &m_invisibilityEnabled)) { m_hack.toggleInvisibility(m_invisibilityEnabled); }
        if (ImGui::Checkbox("Wall Climb", &m_wallClimbEnabled)) { m_hack.toggleWallClimb(m_wallClimbEnabled); }
        if (ImGui::Checkbox("Clipping", &m_clippingEnabled)) { m_hack.toggleClipping(m_clippingEnabled); }
        ImGui::Spacing();
    }

    // -- Actions Section --
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        float button_width = ImGui::GetContentRegionAvail().x * 0.48f;
        if (ImGui::Button("Save Position", ImVec2(button_width, 0))) { m_hack.savePosition(); }
        ImGui::SameLine();
        if (ImGui::Button("Load Position", ImVec2(-1.0f, 0))) { m_hack.loadPosition(); } // Use remaining width
        ImGui::Spacing();
    }

    // -- Hold Keys Status Section --
    if (ImGui::CollapsingHeader("Hold Keys Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Check Super Sprint key
        bool superSprintKeyPressed = (GetAsyncKeyState(KEY_SUPER_SPRINT) & 0x8000) != 0;
        if (superSprintKeyPressed != m_superSprintEnabled) {
            m_superSprintEnabled = superSprintKeyPressed;
            m_hack.handleSuperSprint(m_superSprintEnabled);
        }
        ImGui::Text("Super Sprint (%s): %s", "Num+", m_superSprintEnabled ? "Active" : "Inactive");

        // Check Fly key
        bool flyKeyPressed = (GetAsyncKeyState(KEY_FLY) & 0x8000) != 0;
        if (flyKeyPressed != m_flyEnabled) {
            m_flyEnabled = flyKeyPressed;
            m_hack.handleFly(m_flyEnabled);
        }
        ImGui::Text("Fly (%s):          %s", "Ctrl", m_flyEnabled ? "Active" : "Inactive");

        // Check Sprint key
        bool sprintKeyPressed = (GetAsyncKeyState(KEY_SPRINT) & 0x8000) != 0;
        m_hack.handleSprint(m_sprintEnabled && sprintKeyPressed); // Logic handled by Hack class
        ImGui::Text("Sprint (%s):       %s", "Shift", (m_sprintEnabled && sprintKeyPressed) ? "Active" : "Inactive");
        ImGui::Spacing();
    }

    // -- Log Section --
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_None)) { // Default closed
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lock(g_statusMutex); // Lock for accessing shared message vector
            for (const auto& msg : g_statusMessages) {
                // Apply simple coloring based on message prefix
                ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                if (msg.rfind("ERROR:", 0) == 0)      color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                else if (msg.rfind("WARN:", 0) == 0)  color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
                else if (msg.rfind("INFO:", 0) == 0)  color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                ImGui::TextColored(color, "%s", msg.c_str());
            }
            // Auto-scroll if near the bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2) {
                ImGui::SetScrollHereY(1.0f);
            }
        } // Mutex lock released here
        ImGui::EndChild();

        if (ImGui::Button("Clear Log")) {
            std::lock_guard<std::mutex> lock(g_statusMutex);
            g_statusMessages.clear();
        }
        ImGui::Spacing();
    }

    // -- Info Section --
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        ImGui::Text("Hold Key Reminders:");
        ImGui::BulletText("Super Sprint: NUMPAD+");
        ImGui::BulletText("Fly: Left CTRL");
        ImGui::BulletText("Sprint: Left SHIFT (if toggle enabled)");
        ImGui::Separator();
        ImGui::Text("Consider the paid version at kxtools.xyz!");
        ImGui::Spacing();
    }

    ImGui::End(); // End main window

    return exit_requested;
}