#pragma once

#include <string> // Required by vector below, good practice
#include <vector> // For potential use with status messages later if needed

// Forward declare Hack class
class Hack;

class HackGUI {
public:
    HackGUI(Hack& hack);

    // Renders the ImGui UI. Returns true if exit is requested.
    bool renderUI();

private:
    Hack& m_hack; // Reference to the core application logic

    // Feature states managed by the GUI
    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = true; // Default sprint key check ON
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;

    // Hold-key states (updated each frame)
    bool m_superSprintEnabled = false;
    bool m_flyEnabled = false;

    // Sprint Toggle State
    bool m_sprintActive = false; // Is sprint currently toggled ON?
};