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

    // Pattern scans
    uintptr_t m_baseAddress;
    uintptr_t m_fogAddress;
    uintptr_t m_objectClippingAddress;
    uintptr_t m_betterMovementAddress;

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
    bool m_flyCheck;
    byte m_objectClipping;
    short m_fog;
    byte m_betterMovement;
    int m_speedFreeze;

    // Hack functions
    void initializeOffsets();
    void findProcess();
    void performBaseScan();
    void scanForPatterns();
    void refreshAddresses();
    void readXYZ();
    void writeXYZ(float xValue, float yValue, float zValue);
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

    // Console color management
    HANDLE m_consoleHandle;
    enum ConsoleColor {
        BLUE = 3,
        DEFAULT = 7,
        GREEN = 10,
        RED = 12
    };
};