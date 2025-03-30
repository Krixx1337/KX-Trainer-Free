#pragma once

#include "hack.h"
#include <vector> // For status messages
#include <string> // For status messages

class HackGUI {
public:
    HackGUI(Hack& hack);

    // New method to render the ImGui UI
    void renderUI();

    // Keep state variables public temporarily for easier access from main loop if needed,
    // or make helper functions/keep logic inside renderUI
    // public: // Or private:
    Hack& m_hack;

    // Feature states
    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = false;
    bool m_superSprintEnabled = false;
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;
    bool m_flyEnabled = false;

private:
    // Removed console-specific methods and members
    // Removed check* methods, logic will move to renderUI or main loop

    // Maybe add helpers here later if needed
};
