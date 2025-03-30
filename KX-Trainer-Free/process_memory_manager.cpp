#define NOMINMAX

#include "process_memory_manager.h"
#include "status_ui.h" // For logging - TODO: Replace with dedicated logger interface
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <windows.h>
#include <algorithm>
#include <cstdint>

// Helper to convert wstring to UTF-8 string using Windows API
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) {
        return std::string(); // Error
    }
    std::string strTo(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    if (bytes_converted <= 0) {
        return std::string(); // Error
    }
    return strTo;
}


// Helper to format addresses for logging
std::string PMM_to_hex_string(uintptr_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << address;
    return oss.str();
}


ProcessMemoryManager::~ProcessMemoryManager() {
    Detach();
}

bool ProcessMemoryManager::Attach(const wchar_t* processName) {
    if (IsAttached()) {
        // LogStatus("INFO: Already attached to process ID: " + std::to_string(m_processId));
        return true; // Already attached is not an error
    }

    m_processId = FindProcessId(processName);
    if (m_processId == 0) {
        LogError("Attach failed: Process not found: " + WStringToString(processName), false);
        return false;
    }

    // Using PROCESS_ALL_ACCESS for simplicity; consider least privilege if needed.
    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
    if (m_processHandle == nullptr) {
        LogError("Attach failed: OpenProcess failed for PID: " + std::to_string(m_processId));
        m_processId = 0;
        return false;
    }

    LogStatus("INFO: Successfully attached to process ID: " + std::to_string(m_processId));
    return true;
}

void ProcessMemoryManager::Detach() {
    if (m_processHandle != nullptr && m_processHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_processHandle);
        LogStatus("INFO: Detached from process ID: " + std::to_string(m_processId));
    }
    m_processHandle = nullptr;
    m_processId = 0;
}

bool ProcessMemoryManager::IsAttached() const {
    // Basic check; does not guarantee the process is still responsive.
    return m_processHandle != nullptr && m_processHandle != INVALID_HANDLE_VALUE;
}

uintptr_t ProcessMemoryManager::ResolvePointerChain(uintptr_t baseAddress, const std::vector<unsigned int>& offsets) const {
    if (!IsAttached()) {
        // LogError("ResolvePointerChain failed: Process not attached."); // Potentially spammy
        return 0;
    }

    uintptr_t currentAddress = baseAddress;
    for (size_t i = 0; i < offsets.size(); ++i) {
        if (!Read<uintptr_t>(currentAddress, currentAddress)) {
            LogError("ResolvePointerChain failed: Read error at level " + std::to_string(i) + " (address " + PMM_to_hex_string(currentAddress) + ")");
            return 0;
        }
        // Depending on target, null pointers in the chain might be an error.
        if (currentAddress == 0) {
            LogError("ResolvePointerChain failed: Null pointer encountered at level " + std::to_string(i));
            return 0;
        }
        currentAddress += offsets[i];
    }
    return currentAddress;
}


uintptr_t ProcessMemoryManager::ScanPatternModule(const wchar_t* moduleName, const char* pattern, const char* mask) const {
    if (!IsAttached()) return 0;

    MODULEENTRY32 moduleInfo = GetModuleInfo(moduleName);
    if (moduleInfo.th32ModuleID == 0) {
        LogError("ScanPatternModule failed: Module not found: " + WStringToString(moduleName), false);
        return 0;
    }

    uintptr_t begin = reinterpret_cast<uintptr_t>(moduleInfo.modBaseAddr);
    uintptr_t end = begin + moduleInfo.modBaseSize;

    return ScanPatternRange(begin, end, pattern, mask);
}


uintptr_t ProcessMemoryManager::ScanPatternRange(uintptr_t begin, uintptr_t end, const char* pattern, const char* mask) const {
    if (!IsAttached() || begin >= end) return 0; // Basic validation

    size_t patternLength = strlen(mask);
    if (patternLength == 0) return 0;

    const size_t chunkSize = 4096;
    std::vector<char> buffer(chunkSize);
    uintptr_t currentChunkBase = begin;

    while (currentChunkBase < end) {
        uint64_t remainingBytes = static_cast<uint64_t>(end - currentChunkBase);
        uint64_t readAmount = std::min(static_cast<uint64_t>(chunkSize), remainingBytes); // Use std::min directly
        SIZE_T bytesToRead = static_cast<SIZE_T>(readAmount);

        if (bytesToRead == 0) break;

        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(m_processHandle, reinterpret_cast<LPCVOID>(currentChunkBase), buffer.data(), bytesToRead, &bytesRead)) {
            // Failed to read this chunk (e.g., PAGE_NOACCESS); log and skip to the next potential chunk.
            // LogError("ScanPatternRange info: ReadProcessMemory failed for chunk at " + PMM_to_hex_string(currentChunkBase) + ". Skipping chunk.", true); // Optional detailed logging
            currentChunkBase += bytesToRead;
            continue;
        }

        if (bytesRead == 0) {
            // Read succeeded but returned 0 bytes? Unusual, stop scanning.
            // LogError("ScanPatternRange warning: Read 0 bytes successfully at " + PMM_to_hex_string(currentChunkBase) + ", stopping scan.", false); // Optional detailed logging
            break;
        }

        // Scan the buffer content that was actually read.
        const char* foundInternal = ScanPatternInternal(buffer.data(), bytesRead, pattern, mask);

        if (foundInternal != nullptr) {
            // Found the pattern; calculate address in the target process.
            uintptr_t offsetInBuffer = static_cast<uintptr_t>(foundInternal - buffer.data());
            return currentChunkBase + offsetInBuffer;
        }

        // Pattern not found in this chunk, advance. Overlap scan regions to handle patterns spanning chunks.
        if (bytesRead > (patternLength - 1)) {
            currentChunkBase += (bytesRead - (patternLength - 1));
        }
        else {
            // Chunk smaller than pattern; advance fully to prevent infinite loops.
            currentChunkBase += bytesRead;
        }

        // Optimization: Stop if remaining memory is smaller than the pattern.
        if (currentChunkBase >= end || (end - currentChunkBase) < patternLength) {
            break;
        }
    }

    return 0; // Pattern not found in the specified range
}

bool ProcessMemoryManager::Nop(uintptr_t address, size_t size) const {
    if (!IsAttached() || size == 0) return false;

    std::vector<std::byte> nopArray(size);
    memset(nopArray.data(), 0x90, size); // x86 NOP instruction

    return Patch(address, nopArray.data(), size);
}

bool ProcessMemoryManager::Patch(uintptr_t address, const void* data, size_t size) const {
    if (!IsAttached() || data == nullptr || size == 0) return false;

    DWORD oldProtect = 0;
    // Temporarily change protection to allow writing to executable memory.
    if (!VirtualProtectEx(m_processHandle, reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LogError("Patch failed: VirtualProtectEx (pre-write) failed at address " + PMM_to_hex_string(address));
        return false;
    }

    SIZE_T bytesWritten = 0;
    bool success = WriteProcessMemory(m_processHandle, reinterpret_cast<LPVOID>(address), data, size, &bytesWritten);

    if (!success) {
        LogError("Patch failed: WriteProcessMemory failed at address " + PMM_to_hex_string(address));
    }

    // Attempt to restore original protection regardless of write success.
    DWORD tempProtect;
    if (!VirtualProtectEx(m_processHandle, reinterpret_cast<LPVOID>(address), size, oldProtect, &tempProtect)) {
        LogError("Patch warning: VirtualProtectEx (post-write restore) failed at address " + PMM_to_hex_string(address));
        // Write might have succeeded, but protection restore failed. Consider this a partial failure?
    }

    return success && (bytesWritten == size);
}


// --- Private Helper Implementations ---

DWORD ProcessMemoryManager::FindProcessId(const wchar_t* processName) const {
    PROCESSENTRY32 procEntry = { sizeof(PROCESSENTRY32) };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        // LogError("FindProcessId failed: CreateToolhelp32Snapshot failed.");
        return 0;
    }
    // RAII wrapper for snapshot handle recommended for production code
    // struct SnapshotHandle { HANDLE h; ~SnapshotHandle() { if(h != INVALID_HANDLE_VALUE) CloseHandle(h); } } snapshot { hSnapshot };

    DWORD pid = 0;
    if (Process32FirstW(hSnapshot, &procEntry)) {
        do {
            if (wcscmp(procEntry.szExeFile, processName) == 0) {
                pid = procEntry.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &procEntry));
    }
    else {
        // LogError("FindProcessId failed: Process32FirstW failed.");
    }

    CloseHandle(hSnapshot);
    return pid;
}

MODULEENTRY32 ProcessMemoryManager::GetModuleInfo(const wchar_t* moduleName) const {
    MODULEENTRY32 modEntry = { sizeof(MODULEENTRY32) };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_processId);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        // LogError("GetModuleInfo failed: CreateToolhelp32Snapshot failed for PID: " + std::to_string(m_processId));
        modEntry.th32ModuleID = 0;
        return modEntry;
    }

    bool found = false;
    if (Module32FirstW(hSnapshot, &modEntry)) {
        do {
            if (wcscmp(modEntry.szModule, moduleName) == 0) {
                found = true;
                break;
            }
        } while (Module32NextW(hSnapshot, &modEntry));
    }
    else {
        // LogError("GetModuleInfo failed: Module32FirstW failed for PID: " + std::to_string(m_processId));
    }

    CloseHandle(hSnapshot);
    if (!found) modEntry.th32ModuleID = 0; // Ensure failure indication if not found
    return modEntry;
}


const char* ProcessMemoryManager::ScanPatternInternal(const char* base, size_t size, const char* pattern, const char* mask) const {
    size_t patternLength = strlen(mask);
    if (patternLength == 0 || size < patternLength) {
        return nullptr;
    }

    for (size_t i = 0; i <= size - patternLength; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLength; ++j) {
            if (mask[j] != '?' && pattern[j] != *(base + i + j)) {
                found = false;
                break;
            }
        }
        if (found) {
            return (base + i);
        }
    }
    return nullptr;
}

// --- Logging Wrappers ---
void ProcessMemoryManager::LogStatus(const std::string& message) const {
    // TODO: Replace with a call to a dedicated logger if created
    StatusUI::AddMessage(message);
}

void ProcessMemoryManager::LogError(const std::string& message, bool includeWinError) const {
    std::string fullMessage = "ERROR (PMM): " + message;
    if (includeWinError) {
        DWORD errorCode = GetLastError();
        if (errorCode != 0) {
            LPSTR messageBuffer = nullptr;
            // Use FORMAT_MESSAGE_IGNORE_INSERTS for safety
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

            if (messageBuffer) {
                std::string winError(messageBuffer, size);
                LocalFree(messageBuffer);
                // Trim trailing whitespace/newlines
                while (!winError.empty() && isspace(static_cast<unsigned char>(winError.back()))) {
                    winError.pop_back();
                }
                fullMessage += " (WinError " + std::to_string(errorCode) + ": " + winError + ")";
            }
            else {
                fullMessage += " (WinError " + std::to_string(errorCode) + ": Failed to format message)";
            }
        }
        // Optional: else { fullMessage += " (No Windows error code)"; }
    }
    // TODO: Replace with a call to a dedicated logger if created
    StatusUI::AddMessage(fullMessage);
}