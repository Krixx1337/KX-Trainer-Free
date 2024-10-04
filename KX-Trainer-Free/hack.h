#pragma once

#include <vector>
#include <windows.h>
#include <functional>
#include <string>

class Hack {
public:
    Hack(std::function<void(const std::string&)> statusCallback);
    ~Hack();

    // Hack features
    void refreshAddresses();
    void toggleFog(bool enable);
    void toggleObjectClipping(bool enable);
    void toggleFullStrafe(bool enable);
    void handleSprint(bool enable);
    void handleSuperSprint(bool enable);
    void savePosition();
    void loadPosition();
    void toggleInvisibility(bool enable);
    void toggleWallClimb(bool enable);
    void toggleClipping(bool enable);
    void handleFly(bool enable);

private:
    // Process-related members
    DWORD m_processId;
    HANDLE m_processHandle;

    // Pattern scans
    uintptr_t m_baseAddress;
    uintptr_t m_fogAddress;
    uintptr_t m_objectClippingAddress;
    uintptr_t m_fullStrafeAddress;

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
    byte m_objectClipping;
    byte m_fog;
    byte m_fullStrafe;
    int m_speedFreeze;

    // Hack functions
    void initializeOffsets();
    void findProcess();
    void performBaseScan();
    void scanForPatterns();
    void readXYZ();
    void writeXYZ(float xValue, float yValue, float zValue);
    uintptr_t refreshAddr(const std::vector<unsigned int>& offsets);
    void reportStatus(const std::string& message);

    std::function<void(const std::string&)> m_statusCallback;
};