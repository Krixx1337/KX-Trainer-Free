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
    // Initialize only members that don't require process interaction yet.
    initializeOffsets();
    // Actual process attachment and scanning happens in Initialize()
}

Hack::~Hack() {
    reportStatus("INFO: Shutting down KX Trainer.");
    // Ensure detachment if Initialize() failed partially or wasn't called
    if (m_memoryManager.IsAttached()) {
        m_memoryManager.Detach();
    }
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

bool Hack::Initialize() {
    reportStatus("INFO: Starting KX Trainer initialization...");
    try {
        findProcess();        // Attach to the game process
        performBaseScan();    // Find the base pointer location
        scanForPatterns();    // Find addresses for specific features

        // Initialize cached bytes by reading initial game state
        // These reads are less critical, failure here doesn't stop initialization
        // but might affect initial state reporting in the UI.
        reportStatus("INFO: Reading initial feature states...");
        if (m_fogAddress) {
            if (!m_memoryManager.Read<byte>(m_fogAddress, m_fogByte)) {
                reportStatus("WARN: Failed to read initial fog state byte at " + to_hex_string(m_fogAddress));
            }
            else {
                reportStatus("DEBUG: Initial Fog byte read: " + to_hex_string(m_fogByte));
            }
        }
        if (m_objectClippingAddress) {
            if (!m_memoryManager.Read<byte>(m_objectClippingAddress, m_objectClippingByte)) {
                reportStatus("WARN: Failed to read initial object clipping state byte at " + to_hex_string(m_objectClippingAddress));
            }
            else {
                reportStatus("DEBUG: Initial Object Clipping byte read: " + to_hex_string(m_objectClippingByte));
            }
        }
        if (m_fullStrafeAddress) {
            if (!m_memoryManager.Read<byte>(m_fullStrafeAddress, m_fullStrafeByte)) {
                reportStatus("WARN: Failed to read initial full strafe state byte at " + to_hex_string(m_fullStrafeAddress));
            }
            else {
                reportStatus("DEBUG: Initial Full Strafe byte read: " + to_hex_string(m_fullStrafeByte));
            }
        }

        reportStatus("INFO: Initialization successful.");
        return true;
    }
    catch (const HackInitializationError& e) {
        // Error already reported by the function that threw it.
        // Ensure detachment before returning failure.
        if (m_memoryManager.IsAttached()) m_memoryManager.Detach();
        return false;
    }
    catch (const std::exception& e) {
        reportStatus("ERROR: An unexpected standard error occurred during initialization - " + std::string(e.what()));
        if (m_memoryManager.IsAttached()) m_memoryManager.Detach();
        return false;
    }
    catch (...) {
        reportStatus("ERROR: An unknown error occurred during initialization.");
        if (m_memoryManager.IsAttached()) m_memoryManager.Detach();
        return false;
    }
}


void Hack::findProcess() {
    reportStatus("INFO: Searching for process: " + std::string(GW2_PROCESS_NAME_A));
    if (!m_memoryManager.Attach(GW2_PROCESS_NAME_W)) {
        std::string errorMsg;
        // Check if PID is 0 (process not found) vs other attach errors (permissions?)
        if (m_memoryManager.GetProcessId() == 0) {
            errorMsg = "Failed to find process '" + std::string(GW2_PROCESS_NAME_A) + "'. Is the game running?";
        }
        else {
            errorMsg = "Failed to attach to process '" + std::string(GW2_PROCESS_NAME_A) + "' (PID: " + std::to_string(m_memoryManager.GetProcessId()) + "). Ensure trainer is run as Administrator and check anti-virus/anti-cheat interference.";
        }
        reportStatus("ERROR: " + errorMsg); // Log the specific error
        throw HackInitializationError(errorMsg); // Throw specific error
    }
    reportStatus("INFO: Successfully attached to process (PID: " + std::to_string(m_memoryManager.GetProcessId()) + ").");
}

void Hack::performBaseScan() {
    reportStatus("INFO: Starting base address location scan...");
    int scans = 0;
    bool locationFound = false;
    m_baseAddressLocation = 0; // Ensure reset before scan

    while (scans < MAX_BASE_SCAN_ATTEMPTS && !locationFound) {
        scans++;
        reportStatus("INFO: Scanning for base address location... (Attempt " + std::to_string(scans) + "/" + std::to_string(MAX_BASE_SCAN_ATTEMPTS) + ")");

        uintptr_t patternMatchAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, BASE_SCAN_PATTERN, BASE_SCAN_MASK);

        if (patternMatchAddress != 0) {
            reportStatus("DEBUG: Base pattern matched at address: " + to_hex_string(patternMatchAddress));
            // Calculate the address where the base pointer value is expected to be stored.
            uintptr_t potentialPtrLocation = patternMatchAddress - POINTER_LOCATION_OFFSET;
            reportStatus("DEBUG: Potential base pointer location: " + to_hex_string(potentialPtrLocation));

            uintptr_t pointerValue = 0;
            // Attempt to read the 8-byte pointer value from the calculated location.
            if (m_memoryManager.Read<uintptr_t>(potentialPtrLocation, pointerValue)) {
                reportStatus("DEBUG: Read pointer value: " + to_hex_string(pointerValue));
                // Sanity check: Ensure the read pointer value is above the minimum threshold.
                if (pointerValue > BASE_ADDRESS_MIN_VALUE) {
                    m_baseAddressLocation = potentialPtrLocation; // Store the valid location
                    reportStatus("INFO: Base address location validated: " + to_hex_string(m_baseAddressLocation) + " (Value: " + to_hex_string(pointerValue) + ")");
                    locationFound = true; // Exit loop after successful validation
                }
                else {
                    // Value read is 0 or too low, likely invalid or not yet initialized by the game.
                    reportStatus("WARN: Pointer location found (" + to_hex_string(potentialPtrLocation) + "), but value (" + to_hex_string(pointerValue) + ") is too low or zero. Game might be loading. Retrying scan...");
                }
            }
            else {
                // Failed to read memory at the location derived from the pattern.
                reportStatus("WARN: Found potential pointer location (" + to_hex_string(potentialPtrLocation) + "), but failed to read the 8-byte value. Retrying scan...");
            }
        }
        else {
            reportStatus("DEBUG: Base pattern not found in attempt " + std::to_string(scans));
        }

        if (!locationFound && scans < MAX_BASE_SCAN_ATTEMPTS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(BASE_SCAN_RETRY_DELAY_MS));
        }
    } // End while loop

    if (!locationFound) {
        std::string errorMsg = "Failed to find or validate base address location after " + std::to_string(MAX_BASE_SCAN_ATTEMPTS) + " attempts. Trainer might be incompatible with the current game version or game hasn't fully loaded.";
        reportStatus("ERROR: " + errorMsg);
        m_baseAddressLocation = 0; // Ensure state reflects failure
        throw HackInitializationError(errorMsg);
    }
    // Success already logged inside the loop
}

void Hack::scanForPatterns()
{
    reportStatus("INFO: Scanning for feature patterns...");
    std::string errorMsg;

    // Fog Scan
    reportStatus("INFO: Scanning for Fog pattern...");
    m_fogAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, FOG_PATTERN, FOG_MASK);
    if (m_fogAddress == 0) { errorMsg = "Failed to find Fog pattern. Trainer might be incompatible with current game version."; reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg); }
    reportStatus("INFO: Fog pattern found at: " + to_hex_string(m_fogAddress));
    m_fogAddress += 0x3; // Apply specific offset for the patch byte relative to pattern start
    reportStatus("INFO: Fog patch address set to: " + to_hex_string(m_fogAddress));

    // Object Clipping Scan
    reportStatus("INFO: Scanning for Object Clipping pattern...");
    m_objectClippingAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK);
    if (m_objectClippingAddress == 0) { errorMsg = "Failed to find Object Clipping pattern. Trainer might be incompatible with current game version."; reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg); }
    reportStatus("INFO: Object Clipping pattern found at: " + to_hex_string(m_objectClippingAddress));
    // No offset needed based on constants.h, address is the patch address
    reportStatus("INFO: Object Clipping patch address set to: " + to_hex_string(m_objectClippingAddress));

    // Full Strafe Scan
    reportStatus("INFO: Scanning for Full Strafe pattern...");
    m_fullStrafeAddress = m_memoryManager.ScanPatternModule(GW2_PROCESS_NAME_W, FULL_STRAFE_PATTERN, FULL_STRAFE_MASK);
    if (m_fullStrafeAddress == 0) { errorMsg = "Failed to find Full Strafe pattern. Trainer might be incompatible with current game version."; reportStatus("ERROR: " + errorMsg); throw HackInitializationError(errorMsg); }
    reportStatus("INFO: Full Strafe pattern found at: " + to_hex_string(m_fullStrafeAddress));
    m_fullStrafeAddress += 0x2; // Apply specific offset for the patch byte relative to pattern start
    reportStatus("INFO: Full Strafe patch address set to: " + to_hex_string(m_fullStrafeAddress));

    reportStatus("INFO: All required feature patterns found successfully.");
}

void Hack::refreshAddresses() {
    if (!m_memoryManager.IsAttached() || m_baseAddressLocation == 0) return;

    uintptr_t oldXAddr = m_xAddr; // Store previous address to detect change/invalidation

    m_xAddr = refreshAddr(m_xOffsets);
    m_yAddr = refreshAddr(m_yOffsets);
    m_zAddr = refreshAddr(m_zOffsets);
    m_zHeight1Addr = refreshAddr(m_zHeight1Offsets); // Invisibility
    m_zHeight2Addr = refreshAddr(m_zHeight2Offsets); // Clipping
    m_gravityAddr = refreshAddr(m_gravityOffsets);   // Fly
    m_speedAddr = refreshAddr(m_speedOffsets);
    m_wallClimbAddr = refreshAddr(m_wallClimbOffsets);

    // Optional: Log if a critical address becomes invalid during gameplay
    if (m_xAddr == 0 && oldXAddr != 0) {
        reportStatus("WARN: X coordinate address resolved to NULL. Pointer chain might be broken (e.g., zoning).");
    }
}

void Hack::readXYZ() {
    if (!m_memoryManager.IsAttached()) return;
    // Add checks for resolved addresses before reading
    if (m_xAddr != 0) {
        if (!m_memoryManager.Read<float>(m_xAddr, m_xValue)) reportStatus("ERROR: Failed to read X value at " + to_hex_string(m_xAddr));
    }
    else reportStatus("WARN: Cannot read X value, address not resolved.");

    if (m_yAddr != 0) {
        if (!m_memoryManager.Read<float>(m_yAddr, m_yValue)) reportStatus("ERROR: Failed to read Y value at " + to_hex_string(m_yAddr));
    }
    else reportStatus("WARN: Cannot read Y value, address not resolved.");

    if (m_zAddr != 0) {
        if (!m_memoryManager.Read<float>(m_zAddr, m_zValue)) reportStatus("ERROR: Failed to read Z value at " + to_hex_string(m_zAddr));
    }
    else reportStatus("WARN: Cannot read Z value, address not resolved.");
}

void Hack::writeXYZ(float xValue, float yValue, float zValue)
{
    if (!m_memoryManager.IsAttached()) return;
    // Add checks for resolved addresses before writing
    if (m_xAddr != 0) {
        if (!m_memoryManager.Write<float>(m_xAddr, xValue)) reportStatus("ERROR: Failed to write X value at " + to_hex_string(m_xAddr));
    }
    else reportStatus("WARN: Cannot write X value, address not resolved.");

    if (m_yAddr != 0) {
        if (!m_memoryManager.Write<float>(m_yAddr, yValue)) reportStatus("ERROR: Failed to write Y value at " + to_hex_string(m_yAddr));
    }
    else reportStatus("WARN: Cannot write Y value, address not resolved.");

    if (m_zAddr != 0) {
        if (!m_memoryManager.Write<float>(m_zAddr, zValue)) reportStatus("ERROR: Failed to write Z value at " + to_hex_string(m_zAddr));
    }
    else reportStatus("WARN: Cannot write Z value, address not resolved.");
}

uintptr_t Hack::refreshAddr(const std::vector<unsigned int>& offsets) {
    // ResolvePointerChain already logs errors internally if it fails
    return m_memoryManager.ResolvePointerChain(m_baseAddressLocation, offsets);
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
            m_yValue += 3.f;
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