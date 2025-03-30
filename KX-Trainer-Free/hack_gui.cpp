#include "hack_gui.h"
#include "hack.h"
#include "constants.h" // Assumed to contain hotkey definitions

#include "imgui/imgui.h"

#include <windows.h>   // For GetAsyncKeyState
#include <string>      // For status messages if used internally
#include <vector>      // For status messages access
#include <mutex>       // For status messages access

// External globals from main.cpp (or equivalent)
extern std::vector<std::string> g_statusMessages;
extern std::mutex g_statusMutex;

// Namespace for hotkeys (adjust if constants.h uses a different structure)
using namespace Constants::Hotkeys;

HackGUI::HackGUI(Hack& hack) : m_hack(hack) {}

bool HackGUI::renderUI()
{
    static bool main_window_open = true; // Controls ImGui window visibility & enables 'x' button
    bool exit_requested = false;

    ImGuiWindowFlags window_flags = 0; // Start with default flags
    ImGui::Begin("KX Trainer", &main_window_open, window_flags);

    if (!main_window_open) { // Check if ImGui 'x' or native 'X' was clicked
        exit_requested = true;
    }

    m_hack.refreshAddresses();

    // -- Toggles Section --
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("No Fog", &m_fogEnabled)) { m_hack.toggleFog(m_fogEnabled); }
        if (ImGui::Checkbox("Object Clipping", &m_objectClippingEnabled)) { m_hack.toggleObjectClipping(m_objectClippingEnabled); }
        if (ImGui::Checkbox("Full Strafe", &m_fullStrafeEnabled)) { m_hack.toggleFullStrafe(m_fullStrafeEnabled); }
        ImGui::Checkbox("Enable Sprint Key", &m_sprintEnabled);
        if (ImGui::Checkbox("Invisibility (Mobs)", &m_invisibilityEnabled)) { m_hack.toggleInvisibility(m_invisibilityEnabled); }
        if (ImGui::Checkbox("Wall Climb", &m_wallClimbEnabled)) { m_hack.toggleWallClimb(m_wallClimbEnabled); }
        if (ImGui::Checkbox("Clipping", &m_clippingEnabled)) { m_hack.toggleClipping(m_clippingEnabled); }
    }

    // -- Actions Section --
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Save Position")) { m_hack.savePosition(); }
        ImGui::SameLine();
        if (ImGui::Button("Load Position")) { m_hack.loadPosition(); }
    }

    // -- Hold Keys Status Section --
    if (ImGui::CollapsingHeader("Hold Keys Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool superSprintKeyPressed = (GetAsyncKeyState(Constants::Hotkeys::KEY_SUPER_SPRINT) & 0x8000) != 0;
        if (superSprintKeyPressed != m_superSprintEnabled) {
            m_superSprintEnabled = superSprintKeyPressed;
            m_hack.handleSuperSprint(m_superSprintEnabled);
        }
        ImGui::Text("Super Sprint (%s): %s", "Num+", m_superSprintEnabled ? "Active" : "Inactive");

        bool flyKeyPressed = (GetAsyncKeyState(KEY_FLY) & 0x8000) != 0;
        if (flyKeyPressed != m_flyEnabled) {
            m_flyEnabled = flyKeyPressed;
            m_hack.handleFly(m_flyEnabled);
        }
        ImGui::Text("Fly (%s): %s", "Ctrl", m_flyEnabled ? "Active" : "Inactive");

        bool sprintKeyPressed = (GetAsyncKeyState(KEY_SPRINT) & 0x8000) != 0;
        m_hack.handleSprint(m_sprintEnabled && sprintKeyPressed); // Logic handled by Hack class
        ImGui::Text("Sprint (%s): %s", "Shift", (m_sprintEnabled && sprintKeyPressed) ? "Active" : "Inactive");
        ImGui::SameLine(); ImGui::TextDisabled("(Toggle must be enabled)");
    }

    // -- Log Section --
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        { // Scope for lock guard
            std::lock_guard<std::mutex> lock(g_statusMutex);
            for (const auto& msg : g_statusMessages) { ImGui::TextUnformatted(msg.c_str()); }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f); // Auto-scroll
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
        if (ImGui::Button("Clear Log")) {
            std::lock_guard<std::mutex> lock(g_statusMutex);
            g_statusMessages.clear();
        }
    }

    // -- Info Section --
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        ImGui::Text("Hotkeys still active for hold actions:");
        ImGui::BulletText("Super Sprint: NUMPAD+");
        ImGui::BulletText("Fly: Left CTRL");
        ImGui::BulletText("Sprint: Left SHIFT (if toggle enabled)");
        ImGui::Separator();
        ImGui::Text("Consider the paid version at kxtools.xyz!");
    }

    ImGui::End(); // End the ImGui window

    return exit_requested;
}