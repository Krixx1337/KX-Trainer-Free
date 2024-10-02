#include "hack_gui.h"
#include "hack.h"
#include "hack_constants.h"
#include <iostream>
#include <thread>

using namespace HackConstants;

HackGUI::HackGUI(Hack& hack) : m_hack(hack)
{
    m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}

void HackGUI::start()
{
    printWelcomeMessage();
    displayInfo();
}

void HackGUI::run() {
    while (true) {
        m_hack.refreshAddresses();
        checkFog();
        checkObjectClipping();
        checkFullStrafe();
        checkSprint();
        checkSuperSprint();
        checkPositionKeys();
        checkInvisibility();
        checkWallClimb();
        checkClipping();
        checkFly();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));  // To avoid high CPU usage
    }
}

void HackGUI::checkFog() {
    if (GetAsyncKeyState(KEY_NO_FOG) & 1) {
        m_fogEnabled = !m_fogEnabled;
        m_hack.toggleFog(m_fogEnabled);
        std::cout << "\nNo Fog: " << (m_fogEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkObjectClipping() {
    if (GetAsyncKeyState(KEY_OBJECT_CLIPPING) & 1) {
        m_objectClippingEnabled = !m_objectClippingEnabled;
        m_hack.toggleObjectClipping(m_objectClippingEnabled);
        std::cout << "\nObject Clipping: " << (m_objectClippingEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkFullStrafe() {
    if (GetAsyncKeyState(KEY_FULL_STRAFE) & 1) {
        m_fullStrafeEnabled = !m_fullStrafeEnabled;
        m_hack.toggleFullStrafe(m_fullStrafeEnabled);
        std::cout << "\nFull Strafe: " << (m_fullStrafeEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkSprint() {
    if (GetAsyncKeyState(KEY_SPRINT) & 1) {
        m_sprintEnabled = !m_sprintEnabled;
        m_hack.handleSprint(m_sprintEnabled);
        std::cout << "\nSprint: " << (m_sprintEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkSuperSprint() {
    bool superSprintKeyPressed = GetAsyncKeyState(KEY_SUPER_SPRINT);
    if (superSprintKeyPressed != m_superSprintEnabled) {
        m_superSprintEnabled = superSprintKeyPressed;
        m_hack.handleSuperSprint(m_superSprintEnabled);
        if (m_superSprintEnabled) {
            std::cout << "\nOn: Super Sprint" << std::endl;
        }
        else {
            std::cout << "\nOff: Super Sprint" << std::endl;
        }
    }
}

void HackGUI::checkPositionKeys() {
    if (GetAsyncKeyState(KEY_SAVEPOS) & 1) {
        m_hack.savePosition();
        std::cout << "\nPosition Saved." << std::endl;
    }
    if (GetAsyncKeyState(KEY_LOADPOS) & 1) {
        m_hack.loadPosition();
        std::cout << "\nPosition Loaded." << std::endl;
    }
}

void HackGUI::checkInvisibility() {
    if (GetAsyncKeyState(KEY_INVISIBILITY) & 1) {
        m_invisibilityEnabled = !m_invisibilityEnabled;
        m_hack.toggleInvisibility(m_invisibilityEnabled);
        std::cout << "\nInvisibility: " << (m_invisibilityEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkWallClimb() {
    if (GetAsyncKeyState(KEY_WALLCLIMB) & 1) {
        m_wallClimbEnabled = !m_wallClimbEnabled;
        m_hack.toggleWallClimb(m_wallClimbEnabled);
        std::cout << "\nWall Climb: " << (m_wallClimbEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkClipping() {
    if (GetAsyncKeyState(KEY_CLIPPING) & 1) {
        m_clippingEnabled = !m_clippingEnabled;
        m_hack.toggleClipping(m_clippingEnabled);
        std::cout << "\nClipping: " << (m_clippingEnabled ? "Enabled" : "Disabled") << std::endl;
    }
}

void HackGUI::checkFly() {
    bool flyKeyPressed = GetAsyncKeyState(KEY_FLY);
    if (flyKeyPressed != m_flyEnabled) {
        m_flyEnabled = flyKeyPressed;
        m_hack.handleFly(m_flyEnabled);
        if (m_flyEnabled) {
            std::cout << "\nOn: Fly" << std::endl;
        }
        else {
            std::cout << "\nOff: Fly" << std::endl;
        }
    }
}

void HackGUI::displayInfo()
{
    std::cout << "[Hotkeys]\n"
        << "NUMPAD1 - Save Position\n"
        << "NUMPAD2 - Load Position\n"
        << "NUMPAD3 - Invisibility (for mobs)\n"
        << "NUMPAD4 - Wall Climb\n"
        << "NUMPAD5 - Clipping\n"
        << "NUMPAD6 - Object Clipping\n"
        << "NUMPAD7 - Full Strafe\n"
        << "NUMPAD8 - Toggle No Fog\n"
        << "NUMPAD+ - Super Sprint (hold)\n"
        << "CTRL - Fly (hold)\n"
        << "SHIFT - Sprint\n"
        << "[Logs]\n";
}

void HackGUI::printWelcomeMessage()
{
    std::cout << "[";
    setConsoleColor(BLUE);
    std::cout << "KX Trainer by Krixx";
    setConsoleColor(DEFAULT);
    std::cout << "]";

    std::cout << "\nIf you like the trainer, consider upgrading to the paid version at ";
    setConsoleColor(BLUE);
    std::cout << "kxtools.xyz";
    setConsoleColor(DEFAULT);
    std::cout << " for more features and support!" << std::endl;

    std::cout << "\nGw2-64.exe process found!\n" << std::endl;
}

void HackGUI::setConsoleColor(int color)
{
    SetConsoleTextAttribute(m_consoleHandle, color);
}
