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

        // Initialize cached bytes (optional, reads initial game state)
        if (m_fogAddress) m_memoryManager.Read<byte>(m_fogAddress, m_fogByte);
        if (m_objectClippingAddress) m_memoryManager.Read<byte>(m_objectClippingAddress, m_objectClippingByte);
        if (m_fullStrafeAddress) m_memoryManager.Read<byte>(m_fullStrafeAddress, m_fullStrafeByte);

        reportStatus("INFO: Initialization successful.");
    }
    catch (const HackInitializationError& e) {
        m_memoryManager.Detach();
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
    reportStatus("INFO: Shutting down KX Trainer.");
}

void Hack::initializeOffsets()
{
    m_xOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x120 };
    m_yOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x128 };
    m_zOffsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x124 };
    m_zHeight1Offsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x118 }; // Invisibility
    m_zHeight2Offsets = { BYTE1, BYTE2, BYTE3, BYTE4, 0x114 }; // Clipping
    m_gravityOffsets = { BYTE1, BYTE2, BYTE3, 0x1FC };         // Fly
    m_speedOffsets = { BYTE1, BYTE2, BYTE3, 0x220 };
    m_wallClimbOffsets = { BYTE1, BYTE2, BYTE3, 0x204 };
}

void Hack::findProcess() {
    reportStatus("INFO: Searching for process: " + std::string(GW2_PROCESS_NAME_A));
    if (!m_memoryManager.Attach(GW2_PROCESS_NAME_W)) {
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
            potentialBaseAddress -= BASE_ADDRESS_OFFSET; // Adjust address relative to pattern start

            if (m_memoryManager.Read<unsigned int>(potentialBaseAddress, baseValue)) {
                if (baseValue > BASE_ADDRESS_MIN_VALUE) { // Sanity check the value at the address
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
    if (m_fogAddress == 0) { errorMsg = "Failed to find Fog pattern."; reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg); }
    m_fogAddress += 0x3; // Apply specific offset for the patch byte relative to pattern start

    m_objectClippingAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK);
    if (m_objectClippingAddress == 0) { errorMsg = "Failed to find Object Clipping pattern."; reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg); }
    // No offset needed for object clipping based on constants.h

    m_fullStrafeAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, FULL_STRAFE_PATTERN, FULL_STRAFE_MASK);
    if (m_fullStrafeAddress == 0) { errorMsg = "Failed to find Full Strafe pattern."; reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg); }
    m_fullStrafeAddress += 0x2; // Apply specific offset for the patch byte relative to pattern start

    reportStatus("INFO: Feature patterns found successfully.");
}

void Hack::refreshAddresses() {
    if (!m_memoryManager.IsAttached() || m_baseAddress == 0) return;

    m_xAddr = refreshAddr(m_xOffsets);
    m_yAddr = refreshAddr(m_yOffsets);
    m_zAddr = refreshAddr(m_zOffsets);
    m_zHeight1Addr = refreshAddr(m_zHeight1Offsets); // Invisibility
    m_zHeight2Addr = refreshAddr(m_zHeight2Offsets); // Clipping
    m_gravityAddr = refreshAddr(m_gravityOffsets);   // Fly
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

// --- Feature Toggles / Handlers ---

void Hack::toggleFog(bool enable) {
    if (!m_memoryManager.IsAttached() || m_fogAddress == 0) return;
    m_fogByte = enable ? NO_FOG_ON : NO_FOG_OFF;
    m_memoryManager.Write<byte>(m_fogAddress, m_fogByte);
}

void Hack::toggleObjectClipping(bool enable) {
    if (!m_memoryManager.IsAttached() || m_objectClippingAddress == 0) return;
    m_objectClippingByte = enable ? OBJECT_CLIPPING_ON : OBJECT_CLIPPING_OFF;
    m_memoryManager.Write<byte>(m_objectClippingAddress, m_objectClippingByte);
}

void Hack::toggleFullStrafe(bool enable) {
    if (!m_memoryManager.IsAttached() || m_fullStrafeAddress == 0) return;
    m_fullStrafeByte = enable ? FULL_STRAFE_ON : FULL_STRAFE_OFF;
    m_memoryManager.Write<byte>(m_fullStrafeAddress, m_fullStrafeByte);
}

void Hack::handleSprint(bool userPrefersSprint) {
    if (!m_memoryManager.IsAttached() || m_speedAddr == 0) return;

    if (userPrefersSprint) {
        if (m_memoryManager.Read<float>(m_speedAddr, m_speed)) {
            // Apply sprint only if not super sprinting and currently near normal speed
            if (m_speed >= (NORMAL_SPEED - 0.1f) && m_speed < (SPRINT_SPEED - 0.1f) && !m_wasSuperSprinting) {
                m_speed = SPRINT_SPEED;
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
            }
        }
        m_wasSprinting = true;
    }
    else {
        // Revert from sprint only if we were sprinting and not super sprinting
        if (m_wasSprinting && !m_wasSuperSprinting) {
            if (m_memoryManager.Read<float>(m_speedAddr, m_speed)) {
                // Revert only if current speed is actually sprint speed
                if (abs(m_speed - SPRINT_SPEED) < 0.1f) {
                    m_speed = NORMAL_SPEED;
                    m_memoryManager.Write<float>(m_speedAddr, m_speed);
                }
            }
        }
        m_wasSprinting = false;
    }
}

void Hack::handleSuperSprint(bool enable) {
    if (!m_memoryManager.IsAttached() || m_speedAddr == 0) return;

    if (enable) {
        if (!m_wasSuperSprinting) { // Key just pressed
            if (m_memoryManager.Read<float>(m_speedAddr, m_speed)) {
                // Save speed only if it's not already super sprint
                if (abs(m_speed - SUPER_SPRINT_SPEED) > 0.1f) {
                    m_savedSpeed = m_speed;
                }
                m_speed = SUPER_SPRINT_SPEED;
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
                m_wasSuperSprinting = true;
            }
            else {
                reportStatus("WARN: Failed read speed before activating Super Sprint.");
                m_speed = SUPER_SPRINT_SPEED; // Try activating anyway
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
                m_wasSuperSprinting = true;
            }
        }
        else { // Key held
            // Re-apply speed just in case it was changed externally
            m_speed = SUPER_SPRINT_SPEED;
            m_memoryManager.Write<float>(m_speedAddr, m_speed);
        }
    }
    else { // Key not pressed
        if (m_wasSuperSprinting) { // Key just released
            if (m_memoryManager.Read<float>(m_speedAddr, m_speed)) {
                // Only restore if current speed is super sprint speed
                if (abs(m_speed - SUPER_SPRINT_SPEED) < 0.1f) {
                    // Restore to a valid speed
                    m_speed = (m_savedSpeed >= NORMAL_SPEED - 0.1f) ? m_savedSpeed : NORMAL_SPEED;
                    m_memoryManager.Write<float>(m_speedAddr, m_speed);
                }
            }
            else {
                reportStatus("WARN: Failed read speed before deactivating Super Sprint, setting to normal.");
                m_speed = NORMAL_SPEED; // Fallback
                m_memoryManager.Write<float>(m_speedAddr, m_speed);
            }
            m_wasSuperSprinting = false;
        }
        // else: Key wasn't pressed, do nothing
    }
}

void Hack::savePosition() {
    readXYZ();
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
    // Basic check if position has ever been saved to non-zero coords
    if (m_xSave != 0.0f || m_ySave != 0.0f || m_zSave != 0.0f) {
        writeXYZ(m_xSave, m_ySave, m_zSave);
        reportStatus("INFO: Position loaded.");
    }
    else {
        reportStatus("WARN: No position saved to load.");
    }
}

void Hack::toggleInvisibility(bool enable) {
    if (!m_memoryManager.IsAttached() || m_zHeight1Addr == 0) return;

    m_isInvisibilityActive = enable; // Update state flag first
    if (enable) {
        m_invisibilityValue = INVISIBILITY_ON;
    }
    else {
        // Nudge Y position slightly on disable - potentially helps refresh visibility
        if (m_yAddr != 0 && m_memoryManager.Read<float>(m_yAddr, m_yValue)) {
            m_yValue += 0.01f;
            m_memoryManager.Write<float>(m_yAddr, m_yValue);
        }
        m_invisibilityValue = INVISIBILITY_OFF;
    }
    m_memoryManager.Write<float>(m_zHeight1Addr, m_invisibilityValue);
}

void Hack::toggleWallClimb(bool enable) {
    if (!m_memoryManager.IsAttached() || m_wallClimbAddr == 0) return;
    m_isWallClimbActive = enable;
    m_wallClimbValue = enable ? WALLCLIMB_SPEED : WALLCLIMB_NORMAL_SPEED;
    m_memoryManager.Write<float>(m_wallClimbAddr, m_wallClimbValue);
}

void Hack::toggleClipping(bool enable) {
    if (!m_memoryManager.IsAttached() || m_zHeight2Addr == 0) return;
    m_isClippingActive = enable;
    m_clippingValue = enable ? CLIPPING_ON : CLIPPING_OFF;
    m_memoryManager.Write<float>(m_zHeight2Addr, m_clippingValue);
}

void Hack::handleFly(bool enable) {
    if (!m_memoryManager.IsAttached() || m_gravityAddr == 0) return;

    m_isFlyingActive = enable; // Update state flag first
    if (enable) {
        if (m_memoryManager.Read<float>(m_gravityAddr, m_flyValue)) {
            // Apply only if not already flying
            if (m_flyValue < (FLY_SPEED - 0.1f)) {
                m_flyValue = FLY_SPEED;
                m_memoryManager.Write<float>(m_gravityAddr, m_flyValue);
            }
        }
        else {
            reportStatus("WARN: Failed read gravity before enabling Fly.");
            m_flyValue = FLY_SPEED; // Attempt write anyway
            m_memoryManager.Write<float>(m_gravityAddr, m_flyValue);
        }
    }
    else { // Key released or not pressed
        // Check wasFlyingActive? Not strictly needed if we read first
        if (m_memoryManager.Read<float>(m_gravityAddr, m_flyValue)) {
            // Only revert if currently at fly speed
            if (abs(m_flyValue - FLY_SPEED) < 0.1f) {
                m_flyValue = FLY_NORMAL_SPEED;
                m_memoryManager.Write<float>(m_gravityAddr, m_flyValue);
            }
        }
        else {
            reportStatus("WARN: Failed read gravity before disabling Fly.");
            m_flyValue = FLY_NORMAL_SPEED; // Attempt write anyway
            m_memoryManager.Write<float>(m_gravityAddr, m_flyValue);
        }
    }
}

// --- State Getters Implementation ---

bool Hack::IsFogEnabled() const {
    return m_fogByte == Constants::Settings::NO_FOG_ON;
}

bool Hack::IsObjectClippingEnabled() const {
    return m_objectClippingByte == Constants::Settings::OBJECT_CLIPPING_ON;
}

bool Hack::IsFullStrafeEnabled() const {
    return m_fullStrafeByte == Constants::Settings::FULL_STRAFE_ON;
}

bool Hack::IsSuperSprinting() const {
    return m_wasSuperSprinting;
}

bool Hack::IsInvisibilityEnabled() const {
    return m_isInvisibilityActive;
}

bool Hack::IsWallClimbEnabled() const {
    return m_isWallClimbActive;
}

bool Hack::IsClippingEnabled() const {
    return m_isClippingActive;
}

bool Hack::IsFlying() const {
    return m_isFlyingActive;
}