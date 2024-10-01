#pragma once
#include <windows.h>

template <typename T>
bool ReadMemory(HANDLE processHandle, uintptr_t address, T& value) {
    return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
}

template <typename T>
bool WriteMemory(HANDLE processHandle, uintptr_t address, const T& value) {
    return WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
}
