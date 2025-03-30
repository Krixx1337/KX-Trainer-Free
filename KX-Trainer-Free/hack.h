#pragma once

#include "process_memory_manager.h"
#include <Windows.h>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

class HackInitializationError : public std::runtime_error {
public:
    HackInitializationError(const std::string& message)
        : std::runtime_error(message) {}
};

class Hack {
public:
    Hack(std::function<void(const std::string&)> statusCallback);
    ~Hack();

    void refreshAddresses(); // Refreshes dynamic memory pointers

    // Feature toggles/handlers
    void toggleFog(bool enable);
    void toggleObjectClipping(bool enable);
    void toggleFullStrafe(bool enable);
    void handleSprint(bool enable);
    void handleSuperSprint(bool enable);
    void toggleInvisibility(bool enable);
    void toggleWallClimb(bool enable);
    void toggleClipping(bool enable);
    void handleFly(bool enable);

    // Position saving/loading
    void savePosition();
    void loadPosition();

private:
    ProcessMemoryManager m_memoryManager;
    std::function<void(const std::string&)> m_statusCallback;

    uintptr_t m_baseAddress = 0; // Found via pattern scan during init
    uintptr_t m_fogAddress = 0;
    uintptr_t m_objectClippingAddress = 0;
    uintptr_t m_fullStrafeAddress = 0;

    // Pointer chain offsets relative to base address
    std::vector<unsigned int> m_xOffsets;
    std::vector<unsigned int> m_yOffsets;
    std::vector<unsigned int> m_zOffsets;
    std::vector<unsigned int> m_zHeight1Offsets;
    std::vector<unsigned int> m_zHeight2Offsets;
    std::vector<unsigned int> m_gravityOffsets;
    std::vector<unsigned int> m_speedOffsets;
    std::vector<unsigned int> m_wallClimbOffsets;

    // Resolved dynamic addresses
    uintptr_t m_xAddr = 0;
    uintptr_t m_yAddr = 0;
    uintptr_t m_zAddr = 0;
    uintptr_t m_zHeight1Addr = 0;
    uintptr_t m_zHeight2Addr = 0;
    uintptr_t m_gravityAddr = 0;
    uintptr_t m_speedAddr = 0;
    uintptr_t m_wallClimbAddr = 0;

    // State variables
    float m_xValue = 0.0f, m_yValue = 0.0f, m_zValue = 0.0f;
    float m_xSave = 0.0f, m_ySave = 0.0f, m_zSave = 0.0f;
    float m_speed = 0.0f, m_savedSpeed = 0.0f;
    bool m_wasSuperSprinting = false;
    bool m_wasSprinting = false;
    float m_invisibility = 0.0f, m_wallClimb = 0.0f, m_clipping = 0.0f, m_fly = 0.0f;
    byte m_objectClipping = 0;
    byte m_fog = 0;
    byte m_fullStrafe = 0;

    // Private helper functions
    void initializeOffsets();
    void findProcess();
    void performBaseScan();
    void scanForPatterns();
    void readXYZ();
    void writeXYZ(float xValue, float yValue, float zValue);
    uintptr_t refreshAddr(const std::vector<unsigned int>& offsets);
    void reportStatus(const std::string& message);
};