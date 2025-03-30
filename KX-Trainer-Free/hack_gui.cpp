#include "hack_gui.h"
#include "hack.h"
#include "constants.h"

// ImGui
#include "imgui/imgui.h"

// Win32 for GetAsyncKeyState (if still needed for held keys)
#include <windows.h>
#include <string> // Include string
#include <vector> // Include vector
#include <mutex> // Include mutex

// Reference global status messages defined in main.cpp
extern std::vector<std::string> g_statusMessages;
extern std::mutex g_statusMutex;

using namespace Constants::Hotkeys;

HackGUI::HackGUI(Hack& hack) : m_hack(hack)
{
    // Constructor is now simpler, no console handle needed
}

// Main UI rendering function using ImGui
void HackGUI::renderUI()
{
    // Start the main window
    ImGui::Begin("KX Trainer");

    // --- Toggles --- 
    if (ImGui::CollapsingHeader("Toggles", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Checkbox("No Fog", &m_fogEnabled)) {
            m_hack.toggleFog(m_fogEnabled);
        }
        if (ImGui::Checkbox("Object Clipping", &m_objectClippingEnabled)) {
            m_hack.toggleObjectClipping(m_objectClippingEnabled);
        }
        if (ImGui::Checkbox("Full Strafe", &m_fullStrafeEnabled)) {
            m_hack.toggleFullStrafe(m_fullStrafeEnabled);
        }
        // Sprint is just a toggle now, actual effect handled separately
        ImGui::Checkbox("Enable Sprint Key", &m_sprintEnabled);
        if (ImGui::Checkbox("Invisibility (Mobs)", &m_invisibilityEnabled)) {
            m_hack.toggleInvisibility(m_invisibilityEnabled);
        }
        if (ImGui::Checkbox("Wall Climb", &m_wallClimbEnabled)) {
            m_hack.toggleWallClimb(m_wallClimbEnabled);
        }
        if (ImGui::Checkbox("Clipping", &m_clippingEnabled)) {
            m_hack.toggleClipping(m_clippingEnabled);
        }
    }

    // --- Actions --- 
    if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Save Position")) {
            m_hack.savePosition();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Position")) {
            m_hack.loadPosition();
        }
    }

    // --- Held Keys --- 
    // We still need to check these keys continuously
    // Note: Consider if GetAsyncKeyState is the best way with a GUI loop
    // It might be better to handle these via Win32 messages if possible,
    // but this mirrors the original logic for now.
    if (ImGui::CollapsingHeader("Hold Keys Status", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Super Sprint (Hold)
        bool superSprintKeyPressed = (GetAsyncKeyState(KEY_SUPER_SPRINT) & 0x8000) != 0;
        if (superSprintKeyPressed != m_superSprintEnabled) {
            m_superSprintEnabled = superSprintKeyPressed;
            m_hack.handleSuperSprint(m_superSprintEnabled);
            // Optional: Add visual feedback in the UI besides the log
        }
        ImGui::Text("Super Sprint (%s): %s", "Num+", m_superSprintEnabled ? "Active" : "Inactive");

        // Fly (Hold)
        bool flyKeyPressed = (GetAsyncKeyState(KEY_FLY) & 0x8000) != 0;
        if (flyKeyPressed != m_flyEnabled) {
            m_flyEnabled = flyKeyPressed;
            m_hack.handleFly(m_flyEnabled);
        }
         ImGui::Text("Fly (%s): %s", "Ctrl", m_flyEnabled ? "Active" : "Inactive");

         // Sprint (Toggle Enabled + Hold Key)
         bool sprintKeyPressed = (GetAsyncKeyState(KEY_SPRINT) & 0x8000) != 0;
         // The actual sprint logic is handled by m_hack, activated only if the toggle is on
         m_hack.handleSprint(m_sprintEnabled && sprintKeyPressed);
         ImGui::Text("Sprint (%s): %s", "Shift", (m_sprintEnabled && sprintKeyPressed) ? "Active" : "Inactive");
         ImGui::SameLine(); ImGui::TextDisabled("(Toggle must be enabled)");
    }

    // --- Status/Log Window --- 
    if (ImGui::CollapsingHeader("Log", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        std::lock_guard<std::mutex> lock(g_statusMutex);
        for (const auto& msg : g_statusMessages) {
            ImGui::TextUnformatted(msg.c_str());
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f); // Auto-scroll

        ImGui::PopStyleVar();
        ImGui::EndChild();
         if (ImGui::Button("Clear Log")) {
             std::lock_guard<std::mutex> lock(g_statusMutex);
             g_statusMessages.clear();
        }
    }

    // --- Info / About --- 
     if (ImGui::CollapsingHeader("Info"))
    {
        ImGui::Text("KX Trainer by Krixx");
        ImGui::Text("GitHub: https://github.com/Krixx1337/KX-Trainer-Free");
        ImGui::Separator();
        ImGui::Text("Hotkeys still active for hold actions:");
        ImGui::BulletText("Super Sprint: NUMPAD+");
        ImGui::BulletText("Fly: Left CTRL");
        ImGui::BulletText("Sprint: Left SHIFT (if toggle enabled)");
         ImGui::Separator();
         ImGui::Text("Consider the paid version at kxtools.xyz for more features!");
    }

    ImGui::End(); // End main window
}
