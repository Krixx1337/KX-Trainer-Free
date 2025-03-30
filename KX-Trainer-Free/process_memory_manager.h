#pragma once
#ifndef PROCESS_MEMORY_MANAGER_H
#define PROCESS_MEMORY_MANAGER_H

#include <Windows.h>
#include <TlHelp32.h> // For module snapshotting
#include <cstdint>    // For uintptr_t
#include <string>
#include <vector>

class ProcessMemoryManager {
public:
    ProcessMemoryManager() = default;
    ~ProcessMemoryManager(); // Ensures Detach() is called

    // Process interaction
    bool Attach(const wchar_t* processName); // Attaches to process by name. Logs errors.
    void Detach();
    bool IsAttached() const;

    // Memory read/write operations
    template <typename T>
    bool Read(uintptr_t address, T& outValue) const; // Returns true on successful read of sizeof(T) bytes
    template <typename T>
    bool Write(uintptr_t address, const T& value) const; // Returns true on successful write of sizeof(T) bytes

    // Pointer chain resolution
    uintptr_t ResolvePointerChain(uintptr_t baseAddress, const std::vector<unsigned int>& offsets) const; // Returns final address or 0 on error

    // Pattern scanning
    uintptr_t ScanPatternModule(const wchar_t* moduleName, const char* pattern, const char* mask) const; // Returns address or 0 if not found/error
    uintptr_t ScanPatternRange(uintptr_t begin, uintptr_t end, const char* pattern, const char* mask) const; // Returns address or 0 if not found/error

    // Memory modification
    bool Nop(uintptr_t address, size_t size) const;   // Replaces bytes with NOPs
    bool Patch(uintptr_t address, const void* data, size_t size) const; // Writes raw data

    // Accessors
    DWORD GetProcessId() const { return m_processId; }
    HANDLE GetProcessHandle() const { return m_processHandle; }


private:
    // Internal helpers
    DWORD FindProcessId(const wchar_t* processName) const;
    MODULEENTRY32 GetModuleInfo(const wchar_t* moduleName) const;
    const char* ScanPatternInternal(const char* base, size_t size, const char* pattern, const char* mask) const;
    void LogStatus(const std::string& message) const; // TODO: Replace with dedicated logger
    void LogError(const std::string& message, bool includeWinError = true) const; // TODO: Replace with dedicated logger

    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;

    // Non-copyable
    ProcessMemoryManager(const ProcessMemoryManager&) = delete;
    ProcessMemoryManager& operator=(const ProcessMemoryManager&) = delete;
};

// Template Implementations
template <typename T>
bool ProcessMemoryManager::Read(uintptr_t address, T& outValue) const {
    if (!IsAttached()) return false;
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(m_processHandle, reinterpret_cast<LPCVOID>(address), &outValue, sizeof(T), &bytesRead)) {
        return false;
    }
    return bytesRead == sizeof(T); // Verify correct number of bytes were read
}

template <typename T>
bool ProcessMemoryManager::Write(uintptr_t address, const T& value) const {
    if (!IsAttached()) return false;
    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(m_processHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &bytesWritten)) {
        return false;
    }
    return bytesWritten == sizeof(T); // Verify correct number of bytes were written
}


#endif // PROCESS_MEMORY_MANAGER_H