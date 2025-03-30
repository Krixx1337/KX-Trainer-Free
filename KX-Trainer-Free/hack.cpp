#include "hack.h"
#include "processtools.h"
#include "patternscan.h"
#include "memoryutils.h"
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

// --- Constructor ---
Hack::Hack(std::function<void(const std::string&)> statusCallback)
    : m_statusCallback(std::move(statusCallback))
{
    reportStatus("INFO: Initializing KX Trainer..."); // Start message
    try {
        initializeOffsets(); // No status needed for this internal setup
        findProcess();       // Reports success/failure internally
        performBaseScan();   // Reports attempts, success/failure internally
        scanForPatterns();   // Reports success/failure internally
        reportStatus("INFO: Initialization successful."); // Final success message
    }
    catch (const HackInitializationError& e) {
        // Error already reported before throwing, just re-throw
        throw;
    }
    catch (const std::exception& e) {
        // Report unexpected standard errors
        reportStatus("ERROR: An unexpected standard error occurred during initialization - " + std::string(e.what()));
        throw HackInitializationError("Unexpected standard error during initialization.");
    }
    catch (...) {
        // Report unknown errors
        reportStatus("ERROR: An unknown error occurred during initialization.");
        throw HackInitializationError("Unknown error during initialization.");
    }
}

// --- Destructor ---
Hack::~Hack() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        // No need to report status on destruction
        m_processHandle = nullptr;
    }
}

// --- Initialization Steps ---

void Hack::initializeOffsets()
{
    // No status reporting needed for internal variable setup
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
    reportStatus("INFO: Searching for process: " + std::string(GW2_PROCESS_NAME_A));
    m_processId = GetProcID(GW2_PROCESS_NAME_W);

    if (m_processId == 0) {
        std::string errorMsg = "Process '" + std::string(GW2_PROCESS_NAME_A) + "' not found. Please ensure the game is running.";
        reportStatus("ERROR: " + errorMsg); // Report failure before throwing
        throw HackInitializationError(errorMsg);
    }
    // Skip intermediate "found, opening handle" message
    // reportStatus("INFO: Process found (PID: " + std::to_string(m_processId) + "). Opening handle...");

    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, m_processId);
    if (m_processHandle == nullptr) {
        DWORD error = GetLastError();
        std::string errorMsg = "Failed to open process handle (Error " + std::to_string(error) + "). Try running as administrator.";
        reportStatus("ERROR: " + errorMsg); // Report failure before throwing
        throw HackInitializationError(errorMsg);
    }

    reportStatus("INFO: Process handle obtained successfully."); // Confirm success of this stage
}

void Hack::performBaseScan() {
    reportStatus("INFO: Starting base address scan...");
    int scans = 0;
    unsigned int baseValue = 0;

    while (scans < MAX_BASE_SCAN_ATTEMPTS) {
        scans++;
        // Report scan attempts as this can take time and reassures the user
        reportStatus("INFO: Scanning for Base Address... (Attempt " + std::to_string(scans) + "/" + std::to_string(MAX_BASE_SCAN_ATTEMPTS) + ")");

        uintptr_t potentialBaseAddress = reinterpret_cast<uintptr_t>(
            PatternScanExModule(m_processHandle, GW2_PROCESS_NAME_W, GW2_PROCESS_NAME_W, BASE_SCAN_PATTERN, BASE_SCAN_MASK)
            );

        if (potentialBaseAddress != 0) {
            potentialBaseAddress -= BASE_ADDRESS_OFFSET;
            // Skip intermediate "potential address found" message
            // reportStatus("INFO: Potential base address found at " + to_hex_string(potentialBaseAddress) + ". Validating...");

            if (ReadMemory(m_processHandle, potentialBaseAddress, baseValue)) {
                if (baseValue > BASE_ADDRESS_MIN_VALUE) {
                    m_baseAddress = potentialBaseAddress;
                    reportStatus("INFO: Base address validated: " + to_hex_string(m_baseAddress)); // Report success
                    return;
                }
                else {
                    // Keep warnings, they might indicate an issue (e.g., not logged in)
                    std::ostringstream oss_val;
                    oss_val << "0x" << std::hex << baseValue;
                    reportStatus("WARN: Base address found, but value (" + oss_val.str() + ") is too low. Retrying scan...");
                }
            }
            else {
                reportStatus("WARN: Base address found, but failed to read validation value. Retrying scan...");
            }
        }
        // No need to report "pattern not found in this attempt" - the attempt counter covers this

        std::this_thread::sleep_for(std::chrono::milliseconds(BASE_SCAN_RETRY_DELAY_MS));
    }

    // If loop finishes, it failed
    std::string errorMsg = "Failed to find or validate base address after maximum attempts. Ensure you are logged into a character.";
    reportStatus("ERROR: " + errorMsg); // Report failure before throwing
    throw HackInitializationError(errorMsg);
}

void Hack::scanForPatterns()
{
    reportStatus("INFO: Scanning for feature patterns...");

    // Fog
    m_fogAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME_W, GW2_PROCESS_NAME_W, FOG_PATTERN, FOG_MASK));
    if (m_fogAddress == 0) {
        std::string errorMsg = "Failed to find Fog pattern. Game version might be incompatible.";
        reportStatus("ERROR: " + errorMsg);
        throw HackInitializationError(errorMsg);
    }
    m_fogAddress += 0x3;
    // reportStatus("INFO: Fog pattern found at " + to_hex_string(m_fogAddress)); // Optional: Can be removed

    // Object Clipping
    m_objectClippingAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME_W, GW2_PROCESS_NAME_W, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK));
    if (m_objectClippingAddress == 0) {
        std::string errorMsg = "Failed to find Object Clipping pattern. Game version might be incompatible.";
        reportStatus("ERROR: " + errorMsg);
        throw HackInitializationError(errorMsg);
    }
    // reportStatus("INFO: Object Clipping pattern found at " + to_hex_string(m_objectClippingAddress)); // Optional: Can be removed

    // Full Strafe
    m_fullStrafeAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME_W, GW2_PROCESS_NAME_W, FULL_STRAFE_PATTERN, FULL_STRAFE_MASK));
    if (m_fullStrafeAddress == 0) {
        std::string errorMsg = "Failed to find Full Strafe pattern. Game version might be incompatible.";
        reportStatus("ERROR: " + errorMsg);
        throw HackInitializationError(errorMsg);
    }
    m_fullStrafeAddress += 0x2;
    // reportStatus("INFO: Full Strafe pattern found at " + to_hex_string(m_fullStrafeAddress)); // Optional: Can be removed

    // Only report overall success for this stage
    reportStatus("INFO: Feature patterns found successfully.");
}

// --- Other Methods ---

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
        m_statusCallback(message); // Pass message to the GUI thread
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

        if (m_speed >= NORMAL_SPEED && m_speed < SPRINT_SPEED) {
            m_speed = SPRINT_SPEED;
            WriteMemory(m_processHandle, m_speedAddr, m_speed);
        }
        m_wasSprinting = true;
    }
    else {
        if (m_wasSprinting) {
            m_speed = NORMAL_SPEED;
            WriteMemory(m_processHandle, m_speedAddr, m_speed);
            m_wasSprinting = false;
        }
    }
}

void Hack::handleSuperSprint(bool enable) {
    if (enable) {
        if (!m_wasSuperSprinting) {
            ReadMemory(m_processHandle, m_speedAddr, m_speed);
            m_savedSpeed = m_speed;
        }
        m_speed = SUPER_SPRINT_SPEED;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
        m_wasSuperSprinting = true;
    }
    else if (m_wasSuperSprinting) {
        m_speed = m_savedSpeed;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
        m_wasSuperSprinting = false;
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
