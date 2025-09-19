#include "../include/dll_injector.h"
#include <iostream>

// 进程相关的工具函数实现

bool IsProcessRunning(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (hProcess == NULL) {
        return false;
    }
    
    DWORD exitCode;
    bool isRunning = GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE;
    CloseHandle(hProcess);
    return isRunning;
}

std::wstring GetProcessName(DWORD processId) {
    std::wstring processName;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    
    if (hProcess != NULL) {
        wchar_t processPath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            std::wstring fullPath(processPath);
            size_t lastSlash = fullPath.find_last_of(L"\\");
            if (lastSlash != std::wstring::npos) {
                processName = fullPath.substr(lastSlash + 1);
            } else {
                processName = fullPath;
            }
        }
        CloseHandle(hProcess);
    }
    
    return processName;
}

bool IsProcessElevated(DWORD processId) {
    bool isElevated = false;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    
    if (hProcess != NULL) {
        HANDLE hToken = NULL;
        if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD size;
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
                isElevated = elevation.TokenIsElevated != 0;
            }
            CloseHandle(hToken);
        }
        CloseHandle(hProcess);
    }
    
    return isElevated;
}

void PrintProcessInfo(DWORD processId) {
    std::wcout << L"Process Information:" << std::endl;
    std::wcout << L"  PID: " << processId << std::endl;
    std::wcout << L"  Name: " << GetProcessName(processId) << std::endl;
    std::wcout << L"  Running: " << (IsProcessRunning(processId) ? L"Yes" : L"No") << std::endl;
    std::wcout << L"  Elevated: " << (IsProcessElevated(processId) ? L"Yes" : L"No") << std::endl;
}