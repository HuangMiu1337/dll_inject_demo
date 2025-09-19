#include "../include/dll_injector.h"

DllInjector::DllInjector() {
}

DllInjector::~DllInjector() {
}

std::vector<DWORD> DllInjector::FindProcessByName(const std::wstring& processName) {
    std::vector<DWORD> processIds;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"Failed to create process snapshot");
        return processIds;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                processIds.push_back(pe32.th32ProcessID);
                LOG_INFO(L"Found process: " << processName << L" (PID: " << pe32.th32ProcessID << L")");
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processIds;
}

bool DllInjector::InjectDll(DWORD processId, const std::wstring& dllPath, const InjectionData& data) {
    LOG_INFO(L"Attempting to inject DLL into process " << processId);

    // 获取进程句柄
    HANDLE hProcess = GetProcessHandle(processId);
    if (!hProcess) {
        LOG_ERROR(L"Failed to open target process");
        return false;
    }

    bool success = false;
    LPVOID pDllPath = nullptr;
    LPVOID pInjectionData = nullptr;
    HANDLE hThread = nullptr;

    do {
        // 在目标进程中分配内存存储DLL路径
        SIZE_T dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
        pDllPath = AllocateMemoryInProcess(hProcess, dllPathSize);
        if (!pDllPath) {
            LOG_ERROR(L"Failed to allocate memory for DLL path");
            break;
        }

        // 写入DLL路径
        if (!WriteDataToProcess(hProcess, pDllPath, dllPath.c_str(), dllPathSize)) {
            LOG_ERROR(L"Failed to write DLL path to target process");
            break;
        }

        // 分配内存存储注入数据
        pInjectionData = AllocateMemoryInProcess(hProcess, sizeof(InjectionData));
        if (!pInjectionData) {
            LOG_ERROR(L"Failed to allocate memory for injection data");
            break;
        }

        // 写入注入数据
        if (!WriteDataToProcess(hProcess, pInjectionData, &data, sizeof(InjectionData))) {
            LOG_ERROR(L"Failed to write injection data to target process");
            break;
        }

        // 获取LoadLibraryW函数地址
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!hKernel32) {
            LOG_ERROR(L"Failed to get kernel32.dll handle");
            break;
        }

        LPTHREAD_START_ROUTINE pLoadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
        if (!pLoadLibraryW) {
            LOG_ERROR(L"Failed to get LoadLibraryW address");
            break;
        }

        // 创建远程线程加载DLL
        hThread = CreateRemoteThreadInProcess(hProcess, pLoadLibraryW, pDllPath);
        if (!hThread) {
            LOG_ERROR(L"Failed to create remote thread");
            break;
        }

        // 等待线程完成
        if (!WaitForRemoteThread(hThread)) {
            LOG_ERROR(L"Remote thread execution failed or timed out");
            break;
        }

        LOG_INFO(L"DLL injection successful");
        success = true;

    } while (false);

    // 清理资源
    if (hThread) CloseHandle(hThread);
    if (pDllPath) VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    if (pInjectionData) VirtualFreeEx(hProcess, pInjectionData, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return success;
}

bool DllInjector::EjectDll(DWORD processId, const std::wstring& dllPath) {
    // 实现DLL卸载逻辑
    LOG_INFO(L"Ejecting DLL from process " << processId);
    // 这里可以实现更复杂的DLL卸载逻辑
    return true;
}

HANDLE DllInjector::GetProcessHandle(DWORD processId) {
    return OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
                      PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
                      FALSE, processId);
}

LPVOID DllInjector::AllocateMemoryInProcess(HANDLE hProcess, SIZE_T size) {
    return VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

bool DllInjector::WriteDataToProcess(HANDLE hProcess, LPVOID address, const void* data, SIZE_T size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(hProcess, address, data, size, &bytesWritten) && bytesWritten == size;
}

HANDLE DllInjector::CreateRemoteThreadInProcess(HANDLE hProcess, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter) {
    return CreateRemoteThread(hProcess, NULL, 0, startAddress, parameter, 0, NULL);
}

bool DllInjector::WaitForRemoteThread(HANDLE hThread, DWORD timeout) {
    DWORD waitResult = WaitForSingleObject(hThread, timeout);
    return waitResult == WAIT_OBJECT_0;
}