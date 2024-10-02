#include "hack.h"
#include "processtools.h"
#include "patternscan.h"
#include "memoryutils.h"
#include "hack_constants.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace HackConstants;

Hack::Hack() :
    m_xOffsets{ BYTE1, BYTE2, BYTE3, BYTE4, 0x120 },
    m_yOffsets{ BYTE1, BYTE2, BYTE3, BYTE4, 0x128 },
    m_zOffsets{ BYTE1, BYTE2, BYTE3, BYTE4, 0x124 },
    m_zHeight1Offsets{ BYTE1, BYTE2, BYTE3, BYTE4, 0x118 },
    m_zHeight2Offsets{ BYTE1, BYTE2, BYTE3, BYTE4, 0x114 },
    m_gravityOffsets{ BYTE1, BYTE2, BYTE3, 0x1FC },
    m_speedOffsets{ BYTE1, BYTE2, BYTE3, 0x220 },
    m_wallClimbOffsets{ BYTE1, BYTE2, BYTE3, 0x204 },
    m_consoleHandle(GetStdHandle(STD_OUTPUT_HANDLE))
{
    findProcess();
    performBaseScan();
    scanForPatterns();
}

Hack::~Hack() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
    }
}

void Hack::start() {
    printWelcomeMessage();
    displayInfo();
}

void Hack::run() {
    while (true) {
        refreshAddresses();
        //toggleFog();
        toggleObjectClipping();
        toggleBetterMovement();
        handleSprint();
        savePosition();
        loadPosition();
        handleSuperSprint();
        toggleInvisibility();
        toggleWallClimb();
        toggleClipping();
        handleFly();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Hack::findProcess() {
    m_processId = GetProcID(GW2_PROCESS_NAME);
    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, m_processId);

    if (m_processHandle == nullptr) {
        int exitTime = 5;
        while (exitTime > 0 && m_processHandle == nullptr) {
            std::cout << "Gw2-64.exe process not found!" << std::endl;
            std::cout << "Exiting in " << exitTime << " seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            exitTime--;
            system("cls");
            m_processId = GetProcID(GW2_PROCESS_NAME);
            m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, m_processId);
        }
        if (exitTime == 0) {
            exit(0);
        }
    }
}

void Hack::performBaseScan() {
    int scans = 0;
    unsigned int baseValue = 0;

    while (m_dynamicPtrBaseAddr == 0 || baseValue <= BASE_ADDRESS_MIN_VALUE) {
        m_baseAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, BASE_SCAN_PATTERN, BASE_SCAN_MASK));

        if (m_baseAddress != 0) {
            m_baseAddress -= BASE_ADDRESS_OFFSET;
        }

        m_dynamicPtrBaseAddr = m_baseAddress;

        if (m_dynamicPtrBaseAddr == 0) {
            system("cls");
            scans++;
            std::cout << "Gw2-64.exe process found!" << std::endl;
            std::cout << "Scanning for Base Address... " << scans << std::endl;
        }
        else {
            system("cls");
            ReadMemory(m_processHandle, m_baseAddress, baseValue);
            std::cout << "Gw2-64.exe process found!" << std::endl;
            std::cout << "Waiting for character selection..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    system("cls");
}

void Hack::scanForPatterns()
{
    m_fogAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, FOG_PATTERN, FOG_MASK));
    m_objectClippingAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, OBJECT_CLIPPING_PATTERN, OBJECT_CLIPPING_MASK));
    m_betterMovementAddress = reinterpret_cast<uintptr_t>(PatternScanExModule(m_processHandle, GW2_PROCESS_NAME, GW2_PROCESS_NAME, BETTER_MOVEMENT_PATTERN, BETTER_MOVEMENT_MASK));
    m_betterMovementAddress += 0x2;
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

void Hack::writeXYZ() {
    WriteMemory(m_processHandle, m_xAddr, m_xValue);
    WriteMemory(m_processHandle, m_yAddr, m_yValue);
    WriteMemory(m_processHandle, m_zAddr, m_zValue);
}

void Hack::displayInfo() {
    std::cout << "[Hotkeys]\n"
        << "NUMPAD1 - Save Position\n"
        << "NUMPAD2 - Load Position\n"
        << "NUMPAD3 - Invisibility (for mobs)\n"
        << "NUMPAD4 - Wall Climb\n"
        << "NUMPAD5 - Clipping\n"
        << "NUMPAD6 - Object Clipping\n"
        << "NUMPAD7 - Better Movement\n"
        //<< "NUMPAD8 - Toggle Fog\n"
        << "NUMPAD+ - Super Sprint (hold)\n"
        << "CTRL - Fly (hold)\n"
        << "SHIFT - Sprint\n"
        << "[Logs]\n";
}

void Hack::printWelcomeMessage() {
    std::cout << "[";
    setConsoleColor(BLUE);
    std::cout << "KX Trainer by Krixx";
    setConsoleColor(DEFAULT);
    std::cout << "]";

    std::cout << "\nIf you like the trainer, consider upgrading to the paid version at ";
    setConsoleColor(BLUE);
    std::cout << "kxtools.xyz";
    setConsoleColor(DEFAULT);
    std::cout << " for more features and support!" << std::endl;

    std::cout << "\nGw2-64.exe process found!\n" << std::endl;
}

void Hack::setConsoleColor(int color) {
    SetConsoleTextAttribute(m_consoleHandle, color);
}

uintptr_t Hack::refreshAddr(const std::vector<unsigned int>& offsets) {
    return FindDMAAddy(m_processHandle, m_dynamicPtrBaseAddr, offsets);
}

void Hack::toggleFog() {
    static bool fogToggle = false;
    if (GetAsyncKeyState(KEY_FOG) & 1) {
        fogToggle = !fogToggle;
        ReadMemory(m_processHandle, m_fogAddress, m_fog);
        if (fogToggle) {
            m_fog = 0;
            setConsoleColor(RED);
            std::cout << "\nFog: Off" << std::endl;
        }
        else {
            m_fog = 1;
            setConsoleColor(GREEN);
            std::cout << "\nFog: On" << std::endl;
        }
        WriteMemory(m_processHandle, m_fogAddress, m_fog);
        setConsoleColor(DEFAULT);
    }
}

void Hack::toggleObjectClipping() {
    static bool objectClippingToggle = false;
    if (GetAsyncKeyState(KEY_OBJECT_CLIPPING) & 1) {
        objectClippingToggle = !objectClippingToggle;
        ReadMemory(m_processHandle, m_objectClippingAddress, m_objectClipping);
        if (objectClippingToggle) {
            m_objectClipping = OBJECT_CLIPPING_ON;
            setConsoleColor(GREEN);
            std::cout << "\nObject Clipping: On" << std::endl;
        }
        else {
            m_objectClipping = OBJECT_CLIPPING_OFF;
            setConsoleColor(RED);
            std::cout << "\nObject Clipping: Off" << std::endl;
        }
        WriteMemory(m_processHandle, m_objectClippingAddress, m_objectClipping);
        setConsoleColor(DEFAULT);
    }
}

void Hack::toggleBetterMovement() {
    static bool movementToggle = false;
    if (GetAsyncKeyState(KEY_BETTER_MOVEMENT) & 1) {
        movementToggle = !movementToggle;
        ReadMemory(m_processHandle, m_betterMovementAddress, m_betterMovement);
        if (movementToggle) {
            m_betterMovement = BETTER_MOVEMENT_ON;
            setConsoleColor(GREEN);
            std::cout << "\nBetter Movement: On" << std::endl;
        }
        else {
            m_betterMovement = BETTER_MOVEMENT_OFF;
            setConsoleColor(RED);
            std::cout << "\nBetter Movement: Off" << std::endl;
        }
        WriteMemory(m_processHandle, m_betterMovementAddress, m_betterMovement);
        setConsoleColor(DEFAULT);
    }
}

void Hack::handleSprint() {
    if (m_speedFreeze == 1) {
        ReadMemory(m_processHandle, m_speedAddr, m_speed);
        if (m_speed < SPRINT_SPEED) {
            m_speed = SPRINT_SPEED;
            WriteMemory(m_processHandle, m_speedAddr, m_speed);
        }
    }

    if (GetAsyncKeyState(KEY_SPRINT) & 1) {
        ReadMemory(m_processHandle, m_speedAddr, m_speed);
        if (m_speed < SPRINT_SPEED) {
            m_speedFreeze = 1;
            setConsoleColor(GREEN);
            std::cout << "\nOn: Sprint" << std::endl;
        }
        else {
            m_speedFreeze = 0;
            m_speed = NORMAL_SPEED;
            WriteMemory(m_processHandle, m_speedAddr, m_speed);
            setConsoleColor(RED);
            std::cout << "\nOff: Sprint" << std::endl;
        }
        setConsoleColor(DEFAULT);
    }
}

void Hack::handleSuperSprint() {
    if (GetAsyncKeyState(KEY_SUPER_SPRINT)) {
        if (!m_turboCheck) {
            ReadMemory(m_processHandle, m_speedAddr, m_speed);
            m_turboSpeed = m_speed;
            setConsoleColor(GREEN);
            std::cout << "\nOn: Super Sprint" << std::endl;
            setConsoleColor(DEFAULT);
        }
        m_speed = SUPER_SPRINT_SPEED;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
        m_turboCheck = true;
    }
    else if (m_turboCheck) {
        m_speed = m_turboSpeed;
        WriteMemory(m_processHandle, m_speedAddr, m_speed);
        m_turboCheck = false;
        setConsoleColor(RED);
        std::cout << "\nOff: Super Sprint" << std::endl;
        setConsoleColor(DEFAULT);
    }
}

void Hack::savePosition() {
    if (GetAsyncKeyState(KEY_SAVEPOS) & 1) {
        readXYZ();
        m_xSave = m_xValue;
        m_ySave = m_yValue;
        m_zSave = m_zValue;
        setConsoleColor(BLUE);
        std::cout << "\nSaved Position" << std::endl;
        setConsoleColor(DEFAULT);
    }
}

void Hack::loadPosition() {
    if (GetAsyncKeyState(KEY_LOADPOS) & 1) {
        if (m_xSave == 0 && m_ySave == 0 && m_zSave == 0) {
            setConsoleColor(BLUE);
            std::cout << "\nYou must first save your position" << std::endl;
            setConsoleColor(DEFAULT);
        }
        else {
            m_xValue = m_xSave;
            m_yValue = m_ySave;
            m_zValue = m_zSave;
            writeXYZ();
            setConsoleColor(BLUE);
            std::cout << "\nLoaded Position" << std::endl;
            setConsoleColor(DEFAULT);
        }
    }
}

void Hack::toggleInvisibility() {
    static bool invisibilityToggle = false;
    if (GetAsyncKeyState(KEY_INVISIBILITY) & 1) {
        invisibilityToggle = !invisibilityToggle;
        ReadMemory(m_processHandle, m_zHeight1Addr, m_invisibility);
        if (invisibilityToggle) {
            m_invisibility = INVISIBILITY_ON;
            WriteMemory(m_processHandle, m_zHeight1Addr, m_invisibility);
            setConsoleColor(GREEN);
            std::cout << "\nOn: Invisibility" << std::endl;
        }
        else {
            ReadMemory(m_processHandle, m_yAddr, m_yValue);
            m_yValue += 3.0f;
            WriteMemory(m_processHandle, m_yAddr, m_yValue);
            m_invisibility = INVISIBILITY_OFF;
            WriteMemory(m_processHandle, m_zHeight1Addr, m_invisibility);
            setConsoleColor(RED);
            std::cout << "\nOff: Invisibility" << std::endl;
        }
        setConsoleColor(DEFAULT);
    }
}

void Hack::toggleWallClimb() {
    static bool wallClimbToggle = false;
    if (GetAsyncKeyState(KEY_WALLCLIMB) & 1) {
        wallClimbToggle = !wallClimbToggle;
        ReadMemory(m_processHandle, m_wallClimbAddr, m_wallClimb);
        if (wallClimbToggle) {
            m_wallClimb = WALLCLIMB_SPEED;
            WriteMemory(m_processHandle, m_wallClimbAddr, m_wallClimb);
            setConsoleColor(GREEN);
            std::cout << "\nOn: Wall Climb" << std::endl;
        }
        else {
            m_wallClimb = WALLCLIMB_NORMAL_SPEED;
            WriteMemory(m_processHandle, m_wallClimbAddr, m_wallClimb);
            setConsoleColor(RED);
            std::cout << "\nOff: Wall Climb" << std::endl;
        }
        setConsoleColor(DEFAULT);
    }
}

void Hack::toggleClipping() {
    static bool clippingToggle = false;
    if (GetAsyncKeyState(KEY_CLIPPING) & 1) {
        clippingToggle = !clippingToggle;
        ReadMemory(m_processHandle, m_zHeight2Addr, m_clipping);
        if (clippingToggle) {
            m_clipping = CLIPPING_ON;
            WriteMemory(m_processHandle, m_zHeight2Addr, m_clipping);
            setConsoleColor(GREEN);
            std::cout << "\nOn: Clipping" << std::endl;
        }
        else {
            m_clipping = CLIPPING_OFF;
            WriteMemory(m_processHandle, m_zHeight2Addr, m_clipping);
            setConsoleColor(RED);
            std::cout << "\nOff: Clipping" << std::endl;
        }
        setConsoleColor(DEFAULT);
    }
}

void Hack::handleFly() {
    ReadMemory(m_processHandle, m_gravityAddr, m_fly);
    bool flyToggle = GetAsyncKeyState(KEY_FLY);

    if (flyToggle) {
        if (!m_flyCheck) {
            if (m_fly < FLY_SPEED) {
                m_fly = FLY_SPEED;
                WriteMemory(m_processHandle, m_gravityAddr, m_fly);
                setConsoleColor(GREEN);
                std::cout << "\nOn: Fly" << std::endl;
                setConsoleColor(DEFAULT);
            }
        }
    }
    else {
        if (!m_flyCheck) {
            if (m_fly == 0) {
                m_flyCheck = true;
            }
            else if (m_fly != FLY_NORMAL_SPEED) {
                m_fly = FLY_NORMAL_SPEED;
                WriteMemory(m_processHandle, m_gravityAddr, m_fly);
                setConsoleColor(RED);
                std::cout << "\nOff: Fly" << std::endl;
                setConsoleColor(DEFAULT);
            }
        }
        else {
            if (m_fly != 0) {
                m_flyCheck = false;
            }
        }
    }
}
