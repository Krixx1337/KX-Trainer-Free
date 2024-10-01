#pragma once

#include <vector>
#include <windows.h>

class Hack {
public:
    Hack();
    ~Hack();

    void start();
    void run();

private:
    // Process-related members
    DWORD m_processId;
    HANDLE m_processHandle;
    uintptr_t m_dynamicPtrBaseAddr;
    static constexpr wchar_t* GW2_PROCESS_NAME = L"Gw2-64.exe";

    // Pattern scans
    uintptr_t m_baseAddress;
    uintptr_t m_fogAddress;
    uintptr_t m_objectClippingAddress;
    uintptr_t m_betterMovementAddress;

    // Patterns and masks
    static inline char BASE_SCAN_PATTERN[] = "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x01";
    static inline char BASE_SCAN_MASK[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    static inline char FOG_PATTERN[] = "\x01\x00\x00\x00\x0F\x11\x44\x24\x48\x0F\x10\x07"; // todo: fix pattern
    static inline char FOG_MASK[] = "?xxxxxxxxxxx";
    static inline char OBJECT_CLIPPING_PATTERN[] = "\xD3\x0F\x29\x54\x24\x60\x0F\x28\xCA";
    static inline char OBJECT_CLIPPING_MASK[] = "?xxxxxxxx";
    static inline char BETTER_MOVEMENT_PATTERN[] = "\x75\x00\x00\x28\x4C\x24\x00\x0F\x59\x8B";
    static inline char BETTER_MOVEMENT_MASK[] = "x??xxx?xxx";

    // Offsets
    static constexpr unsigned int BYTE1 = 0x50, BYTE2 = 0x88, BYTE3 = 0x10, BYTE4 = 0x68;

    // Pointers and Addresses
    std::vector<unsigned int> m_xOffsets;
    std::vector<unsigned int> m_yOffsets;
    std::vector<unsigned int> m_zOffsets;
    std::vector<unsigned int> m_zHeight1Offsets;
    std::vector<unsigned int> m_zHeight2Offsets;
    std::vector<unsigned int> m_gravityOffsets;
    std::vector<unsigned int> m_speedOffsets;
    std::vector<unsigned int> m_wallClimbOffsets;

    uintptr_t m_xAddr;
    uintptr_t m_yAddr;
    uintptr_t m_zAddr;
    uintptr_t m_zHeight1Addr;
    uintptr_t m_zHeight2Addr;
    uintptr_t m_gravityAddr;
    uintptr_t m_speedAddr;
    uintptr_t m_wallClimbAddr;

    // Hack variables
    float m_xValue, m_yValue, m_zValue;
    float m_xSave, m_ySave, m_zSave;
    float m_speed, m_turboSpeed, m_turboCheck;
    float m_invisibility, m_wallClimb, m_clipping, m_fly;
    short m_objectClipping;
    short m_fog;
    byte m_betterMovement;
    int m_speedFreeze;
    bool m_flyCheck;

    // Hack functions
    void findProcess();
    void performBaseScan();
    void refreshAddresses();
    void readXYZ();
    void writeXYZ();
    void displayInfo();
    void printWelcomeMessage();
    uintptr_t refreshAddr(const std::vector<unsigned int>& offsets);
    void setConsoleColor(int color);

    // Hack features
    void toggleFog();
    void toggleObjectClipping();
    void toggleBetterMovement();
    void handleSprint();
    void savePosition();
    void loadPosition();
    void handleSuperSprint();
    void toggleInvisibility();
    void toggleWallClimb();
    void toggleClipping();
    void handleFly();

    // Hotkeys
    static constexpr int KEY_SAVEPOS = 0x61;  // NUMPAD1
    static constexpr int KEY_LOADPOS = 0x62;  // NUMPAD2
    static constexpr int KEY_INVISIBILITY = 0x63;  // NUMPAD3
    static constexpr int KEY_WALLCLIMB = 0x64;  // NUMPAD4
    static constexpr int KEY_CLIPPING = 0x65;  // NUMPAD5
    static constexpr int KEY_OBJECT_CLIPPING = 0x66;  // NUMPAD6
    static constexpr int KEY_BETTER_MOVEMENT = 0x67;  // NUMPAD7
    static constexpr int KEY_FOG = 0x68;  // NUMPAD8
    static constexpr int KEY_SUPER_SPRINT = 0x6B;  // NUMPAD+
    static constexpr int KEY_SPRINT = 0xA0;  // Left Shift
    static constexpr int KEY_FLY = 0xA2;  // Left Ctrl

    // Settings
    static constexpr float SPRINT_SPEED = 12.22f;
    static constexpr float NORMAL_SPEED = 9.1875f;
    static constexpr float SUPER_SPRINT_SPEED = 30.0f;
    static constexpr float FLY_SPEED = 20.0f;
    static constexpr float FLY_NORMAL_SPEED = -40.625f;
    static constexpr float WALLCLIMB_SPEED = 20.0f;
    static constexpr float WALLCLIMB_NORMAL_SPEED = 2.1875f;
    static constexpr short OBJECT_CLIPPING_ON = 4059;
    static constexpr short OBJECT_CLIPPING_OFF = 4051;
    static constexpr float INVISIBILITY_ON = 2.7f;
    static constexpr float INVISIBILITY_OFF = 1.0f;
    static constexpr float CLIPPING_ON = 99999.0f;
    static constexpr float CLIPPING_OFF = 0.0f;

    // Console color management
    HANDLE m_consoleHandle;
    enum ConsoleColor {
        DEFAULT = 7,
        BLUE = 3,
        GREEN = 10,
        RED = 12
    };
};