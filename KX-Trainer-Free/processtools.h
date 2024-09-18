#pragma once
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>

//Get Process ID From an executable name using toolhelp32Snapshot
DWORD GetProcID(wchar_t* exeName);

//Get ModuleEntry from module name, using toolhelp32snapshot
MODULEENTRY32 GetModule(DWORD dwProcID, wchar_t* moduleName);

//FindDMAAddy
uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets);
