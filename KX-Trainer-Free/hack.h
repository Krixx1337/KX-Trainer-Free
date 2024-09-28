#pragma once
#include "processtools.h"
#include "patternscan.h"
#include <vector>
#include <fstream>
#include <windows.h>

class Hack {
public:
    // Main functions
    void find_process();
    void base_scan();
    void start();
    void hacks_loop();

private:
    // Process-related variables
    DWORD processID = GetProcID(L"Gw2-64.exe");
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processID);
    uintptr_t dynamicPtrBaseAddr = 0;

    // Pattern scans
    wchar_t* gw2_name = L"Gw2-64.exe";
    void* baseAddress = nullptr;
    void* fogAddress = nullptr;
    void* objectClippingAddress = nullptr;
    void* betterMovementAddress = nullptr;

    // Patterns
    static inline char BASE_SCAN_PATTERN[] =
        "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x01";
    static inline char BASE_SCAN_MASK[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

    static inline char FOG_PATTERN[] =
        "\x01\x00\x00\x00\x0F\x11\x44\x24\x48\x0F\x10\x07";
    static inline char FOG_MASK[] = "?xxxxxxxxxxx";

    static inline char OBJECT_CLIPPING_PATTERN[] =
        "\xD3\x0F\x29\x54\x24\x60\x0F\x28\xCA";
    static inline char OBJECT_CLIPPING_MASK[] = "?xxxxxxxx";

    static inline char BETTER_MOVEMENT_PATTERN[] =
        "\x75\x00\x00\x28\x4C\x24\x00\x0F\x59\x8B";
    static inline char BETTER_MOVEMENT_MASK[] = "x??xxx?xxx";

    // Offsets
    unsigned int byte1 = 0x50, byte2 = 0x88, byte3 = 0x10, byte4 = 0x68;

    // Pointers and Addresses
    std::vector<unsigned int> x_offsets = { byte1, byte2, byte3, byte4, 0x120 };
    std::vector<unsigned int> y_offsets = { byte1, byte2, byte3, byte4, 0x128 };
    std::vector<unsigned int> z_offsets = { byte1, byte2, byte3, byte4, 0x124 };
    std::vector<unsigned int> zheight1_offsets = { byte1, byte2, byte3, byte4, 0x118 };
    std::vector<unsigned int> zheight2_offsets = { byte1, byte2, byte3, byte4, 0x114 };
    std::vector<unsigned int> gravity_offsets = { byte1, byte2, byte3, 0x1FC };
    std::vector<unsigned int> speed_offsets = { byte1, byte2, byte3, 0x220 };
    std::vector<unsigned int> wallclimb_offsets = { byte1, byte2, byte3, 0x204 };

    uintptr_t x_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, x_offsets);
    uintptr_t y_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, y_offsets);
    uintptr_t z_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, z_offsets);
    uintptr_t zheight1_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, zheight1_offsets);
    uintptr_t zheight2_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, zheight2_offsets);
    uintptr_t gravity_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, gravity_offsets);
    uintptr_t speed_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, speed_offsets);
    uintptr_t wallclimb_addr = FindDMAAddy(hProcess, dynamicPtrBaseAddr, wallclimb_offsets);

    // Hack variables
    float x_value = 0, y_value = 0, z_value = 0;
    float x_save = 0, y_save = 0, z_save = 0;
    float speed = 0, turbo_speed = 0;
    float invisibility = 0, wallclimb = 0, clipping = 0, fly = 0;
    short object_clipping = 0;
    short fog = 0;
    byte better_movement = 0;
    int speed_freeze = 0;
    bool fly_check = false, turbo_check = false;

    // Hack functions
    void refresh_xyz();
    void read_xyz();
    void write_xyz();
    void info();
    uintptr_t refresh_addr(std::vector<unsigned int> offsets);
    void set_color(int color);

    // Hack features
    void refresh_address();
    void hacks_fog();
    void hacks_object_clipping();
    void hacks_better_movement();
    void hacks_sprint();
    void hacks_save_position();
    void hacks_load_position();
    void hacks_super_sprint();
    void hacks_invisibility();
    void hacks_wall_climb();
    void hacks_clipping();
    void hacks_fly();

    // Hotkeys
    int key_savepos = 0x61;  // NUMPAD1
    int key_loadpos = 0x62;  // NUMPAD2
    int key_invisibility = 0x63;  // NUMPAD3
    int key_wallclimb = 0x64;  // NUMPAD4
    int key_clipping = 0x65;  // NUMPAD5
    int key_object_clipping = 0x66;  // NUMPAD6
    int key_better_movement = 0x67;  // NUMPAD7
    int key_fog = 0x68;  // NUMPAD8
    int key_super_sprint = 0x6B;  // NUMPAD+
    int key_sprint = 0xA0;  // Left Shift
    int key_fly = 0xA2;  // Left Ctrl

    // Settings
    float sprint_settings = 12.22;
    float fly_settings = 20.0;
    float wallclimb_settings = 20.0;

    // Console color management
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    enum ConsoleColor {
        DEFAULT = 7,
        BLUE = 3,
        GREEN = 10,
        RED = 12
    };
};
