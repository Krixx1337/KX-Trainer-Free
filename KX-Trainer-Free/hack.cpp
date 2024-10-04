#include "hack.h"
#include "processtools.h"
#include "patternscan.h"
#include "memoryutils.h"
#include "hack_constants.h"
#include <thread>
#include <chrono>

using namespace HackConstants;

Hack::Hack(std::function<void(const std::string&)> statusCallback)
    : m_statusCallback(std::move(statusCallback))
{
    initializeOffsets();
    findProcess();
    performBaseScan();
    scanForPatterns();
}

Hack::~Hack() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
    }
}

void Hack::initializeOffsets()
{
    m_xOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x120 };
    m_yOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x128 };
    m_zOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x124 };
    m_zHeight1Offsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x118 };
    m_zHeight2Offsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x114 };
    m_gravityOffsets = { BYTE1, BYTE2, BYTE3, 0x1FC };
    m_speedOffsets = { BYTE1, BYTE2, BYTE3, 0x220 };
    m_wallClimbOffsets = { BYTE1, BYTE2, BYTE3, 0x204 };
}

void Hack::findProcess() {
    m_processId = GetProcID(GW2_PROCESS_NAME);
    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, m_processId);

    if (m_processHandle == nullptr) {
        reportStatus("Gw2-64.exe process not found! Exiting...");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        exit(0);
    }
}

void Hack::performBaseScan() {
    int scans = 0;
    unsigned int baseValue = 0;

    while (m_baseAddress == 0 || baseValue <= BASE_ADDRESS_MIN_VALUE) {
        m_baseAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, BASE_SCAN_PATTERN, BASE_SCAN_MASK));

        if (m_baseAddress != 0) {
            m_baseAddress -= BASE_ADDRESS_OFFSET;
        }

        if (m_baseAddress == 0) {
            scans++;
            reportStatus("Scanning for Base Address... " + std::to_string(scans));
        }
        else {
            ReadMemory(m_processHandle, m_baseAddress, baseValue);
            reportStatus("Waiting for character selection...");
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Hack::scanForPatterns()
{
    m_fogAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, FOG_PATTERN, FOG_MASK));
    m_fogAddress += 0x3;
    m_objectClippingAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK));
    m_fullStrafeAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, FULL_STRAFE_PATTERN, FULL_STRAFE_MASK));
    m_fullStrafeAddress += 0x2;
}

void Hack::refreshAddresses() {
    m_xAddr = refreshAddr(m_xOffsets);
    m_yAddr = refreshAddr(m_yOffsets);
    m_zAddr = refreshAddr(m_zOffsets);
    m_zHeight1Addr = refreshAddr(m_zHeight1Offsets);
    m_zHeight2Addr = refreshAddr(m_zHeight2Offsets);
    m_gravityAddr = refreshAddr(m_gravityOffsets);
    m_speedAddr = refreshAddr(m_speedOffsets);
    m_wallClimbAddr = refreshAddr(m_wallClimbOffsets);
}

void Hack::readXYZ() {
    ReadMemory(m_processHandle, m_xAddr, m_xValue);
    ReadMemory(m_processHandle, m_yAddr, m_yValue);
    ReadMemory(m_processHandle, m_zAddr, m_zValue);
}

void Hack::writeXYZ(float xValue, float yValue, float zValue)
{
    WriteMemory(m_processHandle, m_xAddr, xValue);
    WriteMemory(m_processHandle, m_yAddr, yValue);
    WriteMemory(m_processHandle, m_zAddr, zValue);
}

uintptr_t Hack::refreshAddr(const std::vector<unsigned int>& offsets) {
    return FindDMAAddy(m_processHandle, m_baseAddress, offsets);
}

void Hack::reportStatus(const std::string& message) {
    if (m_statusCallback) {
        m_statusCallback(message);
    }
}

void Hack::toggleFog(bool enable) {
    ReadMemory(m_processHandle, m_fogAddress, m_fog);
    m_fog = enable ? NO_FOG_ON : NO_FOG_OFF;
    WriteMemory(m_processHandle, m_fogAddress, m_fog);
}

void Hack::toggleObjectClipping(bool enable) {
    ReadMemory(m_processHandle, m_objectClippingAddress, m_objectClipping);
    m_objectClipping = enable ? OBJECT_CLIPPING_ON : OBJECT_CLIPPING_OFF;
    WriteMemory(m_processHandle, m_objectClippingAddress, m_objectClipping);
}

void Hack::toggleFullStrafe(bool enable) {
    ReadMemory(m_processHandle, m_fullStrafeAddress, m_fullStrafe);
    m_fullStrafe = enable ? FULL_STRAFE_ON : FULL_STRAFE_OFF;
    WriteMemory(m_processHandle, m_fullStrafeAddress, m_fullStrafe);
}

void Hack::handleSprint(bool enable) {
    if (enable) {
        ReadMemory(m_processHandle, m_speedAddr, m_speed);
        if (m_speed < SPRINT_SPEED) {
            m_speed = SPRINT_SPEED;
            WriteMemory(m_processHandle, m_speedAddr, m_speed);
        }
        m_speedFreeze = 1;
    }
    else {
        m_speedFreeze = 0;
        m_speed = NORMAL_SPEED;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
    }
}

void Hack::handleSuperSprint(bool enable) {
    if (enable) {
        if (!m_turboCheck) {
            ReadMemory(m_processHandle, m_speedAddr, m_speed);
            m_turboSpeed = m_speed;
        }
        m_speed = SUPER_SPRINT_SPEED;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
        m_turboCheck = true;
    }
    else if (m_turboCheck) {
        m_speed = m_turboSpeed;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
        m_turboCheck = false;
    }
}

void Hack::savePosition() {
    readXYZ();
    m_xSave = m_xValue;
    m_ySave = m_yValue;
    m_zSave = m_zValue;
}

void Hack::loadPosition() {
    if (m_xSave != 0 || m_ySave != 0 || m_zSave != 0) {
        writeXYZ(m_xSave, m_ySave, m_zSave);
    }
}

void Hack::toggleInvisibility(bool enable) {
    ReadMemory(m_processHandle, m_zHeight1Addr, m_invisibility);
    if (enable) {
        m_invisibility = INVISIBILITY_ON;
    }
    else {
        ReadMemory(m_processHandle, m_yAddr, m_yValue);
        m_yValue += 3.0f;
        WriteMemory(m_processHandle, m_yAddr, m_yValue);
        m_invisibility = INVISIBILITY_OFF;
    }
    WriteMemory(m_processHandle, m_zHeight1Addr, m_invisibility);
}

void Hack::toggleWallClimb(bool enable) {
    ReadMemory(m_processHandle, m_wallClimbAddr, m_wallClimb);
    m_wallClimb = enable ? WALLCLIMB_SPEED : WALLCLIMB_NORMAL_SPEED;
    WriteMemory(m_processHandle, m_wallClimbAddr, m_wallClimb);
}

void Hack::toggleClipping(bool enable) {
    ReadMemory(m_processHandle, m_zHeight2Addr, m_clipping);
    m_clipping = enable ? CLIPPING_ON : CLIPPING_OFF;
    WriteMemory(m_processHandle, m_zHeight2Addr, m_clipping);
}

void Hack::handleFly(bool enable) {
    ReadMemory(m_processHandle, m_gravityAddr, m_fly);
    if (enable) {
        if (m_fly < FLY_SPEED) {
            m_fly = FLY_SPEED;
            WriteMemory(m_processHandle, m_gravityAddr, m_fly);
        }
    }
    else {
        if (m_fly != FLY_NORMAL_SPEED) {
            m_fly = FLY_NORMAL_SPEED;
            WriteMemory(m_processHandle, m_gravityAddr, m_fly);
        }
    }
}
