#pragma once

#include "hack.h"

class HackGUI {
public:
    HackGUI(Hack& hack);

    void start();
    void run();

private:
    Hack& m_hack;

    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = false;
    bool m_superSprintEnabled = false;
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;
    bool m_flyEnabled = false;

    void checkFog();
    void checkObjectClipping();
    void checkFullStrafe();
    void checkSprint();
    void checkSuperSprint();
    void checkPositionKeys();
    void checkInvisibility();
    void checkWallClimb();
    void checkClipping();
    void checkFly();

    void displayInfo();
    void printWelcomeMessage();
    void setConsoleColor(int color);

    // Console color management
    HANDLE m_consoleHandle;
    enum ConsoleColor {
        BLUE = 3,
        DEFAULT = 7,
        GREEN = 10,
        RED = 12
    };
};
