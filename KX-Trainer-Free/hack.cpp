#include "hack.h"
#include "constants.h"
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

using namespace Constants::Process;
using namespace Constants::Offsets;
using namespace Constants::Scan;
using namespace Constants::Patterns;
using namespace Constants::Settings;

// Helper to format addresses
std::string to_hex_string(uintptr_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << address;
    return oss.str();
}

Hack::Hack(std::function<void(const std::string&)> statusCallback)
    : m_statusCallback(std::move(statusCallback))
{
    reportStatus("INFO: Initializing KX Trainer...");
    try {
        initializeOffsets();
        findProcess();
        performBaseScan();
        scanForPatterns();
        reportStatus("INFO: Initialization successful.");
    }
    catch (const HackInitializationError& e) {
        m_memoryManager.Detach(); // Ensure detachment on failure
        throw;
    }
    catch (const std::exception& e) {
        reportStatus("ERROR: An unexpected standard error occurred during initialization - " + std::string(e.what()));
        m_memoryManager.Detach();
        throw HackInitializationError("Unexpected standard error during initialization.");
    }
    catch (...) {
        reportStatus("ERROR: An unknown error occurred during initialization.");
        m_memoryManager.Detach();
        throw HackInitializationError("Unknown error during initialization.");
    }
}

Hack::~Hack() {
    // m_memoryManager destructor handles detachment
    reportStatus("INFO: Shutting down KX Trainer.");
}

void Hack::initializeOffsets()
{
    m_xOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x120 };
    m_yOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x128 };
    m_zOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x124 };
    m_zHeight1Offsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x118 }; // Invisibility related
    m_zHeight2Offsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x114 }; // Clipping related
    m_gravityOffsets = { BYTE1, BYTE2, BYTE3, 0x1FC };         // Fly related
    m_speedOffsets = { BYTE1, BYTE2, BYTE3, 0x220 };
    m_wallClimbOffsets = { BYTE1, BYTE2, BYTE3, 0x204 };
}

void Hack::findProcess() {
    reportStatus("INFO: Searching for process: " + std::string(GW2_PROCESS_NAME_A));
    if (!m_memoryManager.Attach(GW2_PROCESS_NAME_W)) {
        // Error logged by Attach
        throw HackInitializationError("Failed to attach to process '" + std::string(GW2_PROCESS_NAME_A) + "'.");
    }
    reportStatus("INFO: Process handle obtained successfully.");
}

void Hack::performBaseScan() {
    reportStatus("INFO: Starting base address scan...");
    int scans = 0;
    unsigned int baseValue = 0;
    bool baseFound = false;

    while (scans < MAX_BASE_SCAN_ATTEMPTS) {
        scans++;
        reportStatus("INFO: Scanning for Base Address... (Attempt " + std::to_string(scans) + "/" + std::to_string(MAX_BASE_SCAN_ATTEMPTS) + ")");

        uintptr_t potentialBaseAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, BASE_SCAN_PATTERN, BASE_SCAN_MASK);

        if (potentialBaseAddress != 0) {
            potentialBaseAddress -= BASE_ADDRESS_OFFSET;

            if (m_memoryManager.Read<unsigned int>(potentialBaseAddress, baseValue)) {
                if (baseValue > BASE_ADDRESS_MIN_VALUE) {
                    m_baseAddress = potentialBaseAddress;
                    reportStatus("INFO: Base address validated: " + to_hex_string(m_baseAddress));
                    baseFound = true;
                    return;
                }
                else {
                    std::ostringstream oss_val; oss_val << "0x" << std::hex << baseValue;
                    reportStatus("WARN: Base address found, but value (" + oss_val.str() + ") is too low. Retrying scan...");
                }
            }
            else {
                reportStatus("WARN: Base address found (" + to_hex_string(potentialBaseAddress) + "), but failed to read validation value. Retrying scan...");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(BASE_SCAN_RETRY_DELAY_MS));
    }

    if (!baseFound) {
        std::string errorMsg = "Failed to find or validate base address after maximum attempts.";
        reportStatus("ERROR: " + errorMsg);
        throw HackInitializationError(errorMsg);
    }
}

void Hack::scanForPatterns()
{
    reportStatus("INFO: Scanning for feature patterns...");
    std::string errorMsg;

    m_fogAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, FOG_PATTERN, FOG_MASK);
    if (m_fogAddress == 0) {
        errorMsg = "Failed to find Fog pattern.";
        reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg);
    }
    m_fogAddress += 0x3;

    m_objectClippingAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK);
    if (m_objectClippingAddress == 0) {
        errorMsg = "Failed to find Object Clipping pattern.";
        reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg);
    }

    m_fullStrafeAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, FULL_STRAFE_PATTERN, FULL_STRAFE_MASK);
    if (m_fullStrafeAddress == 0) {
        errorMsg = "Failed to find Full Strafe pattern.";
        reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg);
    }
    m_fullStrafeAddress += 0x2;

    reportStatus("INFO: Feature patterns found successfully.");
}

void Hack::refreshAddresses() {
    if (!m_memoryManager.IsAttached() || m_baseAddress == 0) return;

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
    if (!m_memoryManager.IsAttached()) return;
    if (m_xAddr != 0) m_memoryManager.Read<float>(m_xAddr, m_xValue);
    if (m_yAddr != 0) m_memoryManager.Read<float>(m_yAddr, m_yValue);
    if (m_zAddr != 0) m_memoryManager.Read<float>(m_zAddr, m_zValue);
}

void Hack::writeXYZ(float xValue, float yValue, float zValue)
{
    if (!m_memoryManager.IsAttached()) return;
    if (m_xAddr != 0) m_memoryManager.Write<float>(m_xAddr, xValue);
    if (m_yAddr != 0) m_memoryManager.Write<float>(m_yAddr, yValue);
    if (m_zAddr != 0) m_memoryManager.Write<float>(m_zAddr, zValue);
}

uintptr_t Hack::refreshAddr(const std::vector<unsigned int>& offsets) {
    return m_memoryManager.ResolvePointerChain(m_baseAddress, offsets);
}

void Hack::reportStatus(const std::string& message) {
    if (m_statusCallback) {
        m_statusCallback(message);
    }
}

void Hack::toggleFog(bool enable) {
    if (!m_memoryManager.IsAttached() || m_fogAddress == 0) return;
    m_memoryManager.Read<byte>(m_fogAddress, m_fog);
    m_fog = enable ? NO_FOG_ON : NO_FOG_OFF;
    m_memoryManager.Write<byte>(m_fogAddress, m_fog);
}

void Hack::toggleObjectClipping(bool enable) {
    if (!m_memoryManager.IsAttached() || m_objectClippingAddress == 0) return;
    m_memoryManager.Read<byte>(m_objectClippingAddress, m_objectClipping);
    m_objectClipping = enable ? OBJECT_CLIPPING_ON : OBJECT_CLIPPING_OFF;
    m_memoryManager.Write<byte>(m_objectClippingAddress, m_objectClipping);
}

void Hack::toggleFullStrafe(bool enable) {
    if (!m_memoryManager.IsAttached() || m_fullStrafeAddress == 0) return;
    m_memoryManager.Read<byte>(m_fullStrafeAddress, m_fullStrafe);
    m_fullStrafe = enable ? FULL_STRAFE_ON : FULL_STRAFE_OFF;
    m_memoryManager.Write<byte>(m_fullStrafeAddress, m_fullStrafe);
}

void Hack::handleSprint(bool enable) {
    if (!m_memoryManager.IsAttached() || m_speedAddr == 0) return;
    if (enable) {
        if (m_memoryManager.Read<float>(m_speedAddr, m_speed)) {
            if (m_speed >= NORMAL_SPEED && m_speed < SPRINT_SPEED) {
                m_speed = SPRINT_SPEED;
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
            }
        }
        m_wasSprinting = true;
    }
    else {
        if (m_wasSprinting) {
            m_speed = NORMAL_SPEED;
            m_memoryManager.Write<float>(m_speedAddr, m_speed);
            m_wasSprinting = false;
        }
    }
}

void Hack::handleSuperSprint(bool enable) {
    if (!m_memoryManager.IsAttached() || m_speedAddr == 0) return;
    if (enable) {
        if (!m_wasSuperSprinting) {
            if (m_memoryManager.Read<float>(m_speedAddr, m_speed)) {
                m_savedSpeed = m_speed;
                m_speed = SUPER_SPRINT_SPEED;
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
                m_wasSuperSprinting = true;
            }
            else {
                // Activate without saving if read fails?
                m_speed = SUPER_SPRINT_SPEED;
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
                m_wasSuperSprinting = true;
            }
        }
        else {
            m_speed = SUPER_SPRINT_SPEED;
            m_memoryManager.Write<float>(m_speedAddr, m_speed);
        }
    }
    else if (m_wasSuperSprinting) {
        m_speed = m_savedSpeed;
        m_memoryManager.Write<float>(m_speedAddr, m_speed);
        m_wasSuperSprinting = false;
    }
}

void Hack::savePosition() {
    readXYZ(); // Populate m_xValue, etc.
    m_xSave = m_xValue;
    m_ySave = m_yValue;
    m_zSave = m_zValue;
    if (m_xAddr != 0 && m_yAddr != 0 && m_zAddr != 0)
        reportStatus("INFO: Position saved.");
    else
        reportStatus("WARN: Position saved, but coordinate addresses might be invalid.");
}

void Hack::loadPosition() {
    if (m_xAddr == 0 || m_yAddr == 0 || m_zAddr == 0) {
        reportStatus("ERROR: Cannot load position, coordinate addresses not resolved.");
        return;
    }
    if (m_xSave != 0.0f || m_ySave != 0.0f || m_zSave != 0.0f) {
        writeXYZ(m_xSave, m_ySave, m_zSave);
        reportStatus("INFO: Position loaded.");
    }
    else {
        reportStatus("WARN: Attempting to load position (0,0,0) or no position saved.");
        // Optionally write 0,0,0
        // writeXYZ(0.0f, 0.0f, 0.0f);
    }
}

void Hack::toggleInvisibility(bool enable) {
    if (!m_memoryManager.IsAttached() || m_zHeight1Addr == 0) return;
    m_memoryManager.Read<float>(m_zHeight1Addr, m_invisibility);

    if (enable) {
        m_invisibility = INVISIBILITY_ON;
    }
    else {
        // Nudge Y position when disabling
        if (m_yAddr != 0) {
            m_memoryManager.Read<float>(m_yAddr, m_yValue);
            m_yValue += 3.0f;
            m_memoryManager.Write<float>(m_yAddr, m_yValue);
        }
        m_invisibility = INVISIBILITY_OFF;
    }
    m_memoryManager.Write<float>(m_zHeight1Addr, m_invisibility);
}

void Hack::toggleWallClimb(bool enable) {
    if (!m_memoryManager.IsAttached() || m_wallClimbAddr == 0) return;
    m_memoryManager.Read<float>(m_wallClimbAddr, m_wallClimb);
    m_wallClimb = enable ? WALLCLIMB_SPEED : WALLCLIMB_NORMAL_SPEED;
    m_memoryManager.Write<float>(m_wallClimbAddr, m_wallClimb);
}

void Hack::toggleClipping(bool enable) {
    if (!m_memoryManager.IsAttached() || m_zHeight2Addr == 0) return;
    m_memoryManager.Read<float>(m_zHeight2Addr, m_clipping);
    m_clipping = enable ? CLIPPING_ON : CLIPPING_OFF;
    m_memoryManager.Write<float>(m_zHeight2Addr, m_clipping);
}

void Hack::handleFly(bool enable) {
    if (!m_memoryManager.IsAttached() || m_gravityAddr == 0) return;
    if (m_memoryManager.Read<float>(m_gravityAddr, m_fly)) {
        if (enable) {
            if (m_fly < FLY_SPEED) {
                m_fly = FLY_SPEED;
                m_memoryManager.Write<float>(m_gravityAddr, m_fly);
            }
        }
        else {
            if (m_fly != FLY_NORMAL_SPEED) {
                m_fly = FLY_NORMAL_SPEED;
                m_memoryManager.Write<float>(m_gravityAddr, m_fly);
            }
        }
    }
}