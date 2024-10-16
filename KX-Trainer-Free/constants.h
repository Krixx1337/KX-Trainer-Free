#pragma once

namespace Constants {
    // Application Info
    constexpr int APP_VERSION = 1010;
    constexpr char APP_NAME[] = "KX Trainer";
    constexpr char API_URL[] = "https://kxtools.xyz/api/v1/version/kx-trainer-free";

    // Process
    namespace Process {
        constexpr wchar_t* GW2_PROCESS_NAME = L"Gw2-64.exe";
    }

    // Scan Configuration
    namespace Scan {
        constexpr unsigned int BASE_ADDRESS_MIN_VALUE = 10000;
        constexpr uintptr_t BASE_ADDRESS_OFFSET = 0x8;
    }

    // Patterns and Masks
    namespace Patterns {
        static inline char BASE_SCAN_PATTERN[] = "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x01";
        static inline char BASE_SCAN_MASK[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
        static inline char FOG_PATTERN[] = "\xF3\x0F\x11\x00\x24\x00\xF3\x0F\x11\x5C\x24\x00\xF3\x0F\x10\x9C\x24";
        static inline char FOG_MASK[] = "xxx?x?xxxxx?xxxxx";
        static inline char OBJECT_CLIPPING_PATTERN[] = "\xD3\x0F\x29\x54\x24\x60\x0F\x28\xCA";
        static inline char OBJECT_CLIPPING_MASK[] = "?xxxxxxxx";
        static inline char FULL_STRAFE_PATTERN[] = "\x75\x00\x00\x28\x4C\x24\x00\x0F\x59\x8B";
        static inline char FULL_STRAFE_MASK[] = "x??xxx?xxx";
    }

    // Offsets
    namespace Offsets {
        constexpr unsigned int BYTE1 = 0x50;
        constexpr unsigned int BYTE2 = 0x88;
        constexpr unsigned int BYTE3 = 0x10;
        constexpr unsigned int BYTE4 = 0x68;
    }

    // Hotkeys
    namespace Hotkeys {
        constexpr int KEY_SAVEPOS = 0x61;  // NUMPAD1
        constexpr int KEY_LOADPOS = 0x62;  // NUMPAD2
        constexpr int KEY_INVISIBILITY = 0x63;  // NUMPAD3
        constexpr int KEY_WALLCLIMB = 0x64;  // NUMPAD4
        constexpr int KEY_CLIPPING = 0x65;  // NUMPAD5
        constexpr int KEY_OBJECT_CLIPPING = 0x66;  // NUMPAD6
        constexpr int KEY_FULL_STRAFE = 0x67;  // NUMPAD7
        constexpr int KEY_NO_FOG = 0x68;  // NUMPAD8
        constexpr int KEY_SUPER_SPRINT = 0x6B;  // NUMPAD+
        constexpr int KEY_SPRINT = 0xA0;  // Left Shift
        constexpr int KEY_FLY = 0xA2;  // Left Ctrl
    }

    // Settings
    namespace Settings {
        constexpr float SPRINT_SPEED = 12.22f;
        constexpr float NORMAL_SPEED = 9.1875f;
        constexpr float SUPER_SPRINT_SPEED = 30.0f;
        constexpr float FLY_SPEED = 20.0f;
        constexpr float FLY_NORMAL_SPEED = -40.625f;
        constexpr float WALLCLIMB_SPEED = 20.0f;
        constexpr float WALLCLIMB_NORMAL_SPEED = 2.1875f;
        constexpr byte OBJECT_CLIPPING_ON = 0xDB;  // xmm3
        constexpr byte OBJECT_CLIPPING_OFF = 0xD3; // xmm2
        constexpr float INVISIBILITY_ON = 2.7f;
        constexpr float INVISIBILITY_OFF = 1.0f;
        constexpr float CLIPPING_ON = 99999.0f;
        constexpr float CLIPPING_OFF = 0.0f;
        constexpr byte FULL_STRAFE_ON = 0x75;  // Jne
        constexpr byte FULL_STRAFE_OFF = 0x0F;  // Movaps
        constexpr byte NO_FOG_ON = 0x54;  // xmm2
        constexpr byte NO_FOG_OFF = 0x5C;  // xmm3
    }
}