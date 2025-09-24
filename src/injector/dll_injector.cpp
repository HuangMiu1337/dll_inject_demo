#include "../include/dll_injector.h"

DllInjector::DllInjector() {
    LOG_DEBUG(L"DllInjector created");
}

DllInjector::~DllInjector() {
    LOG_DEBUG(L"DllInjector destroyed");
}

std::vector<DWORD> DllInjector::FindProcessByName(const std::wstring& processName) {
    std::vector<DWORD> processIds;
    
    // 安全验证
    if (!SecurityUtils::ValidateProcessName(processName)) {
        LOG_ERROR(L"Process name failed security validation: " << processName);
        return processIds;
    }
    
    if (processName.length() > MAX_PATH) {
        LOG_ERROR(L"Process name too long: " << processName.length());
        return processIds;
    }
    
    ScopedHandle<HANDLE> hSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!hSnapshot) {
        DWORD error = GetLastError();
        LOG_ERROR(L"Failed to create process snapshot, error: " << error);
        return processIds;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot.get(), &pe32)) {
        DWORD error = GetLastError();
        LOG_ERROR(L"Failed to enumerate first process, error: " << error);
        return processIds;
    }
    
    do {
        if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
            processIds.push_back(pe32.th32ProcessID);
            LOG_INFO(L"Found process: " << processName << L" (PID: " << pe32.th32ProcessID << L")");
        }
    } while (Process32NextW(hSnapshot.get(), &pe32));

    LOG_INFO(L"Found " << processIds.size() << L" instances of process: " << processName);
    return processIds;
}

ErrorCode DllInjector::InjectDll(DWORD processId, const std::wstring& dllPath, const InjectionData& data) {
    LOG_DEBUG(L"Starting DLL injection for process ID: " << processId);
    
    // 安全验证
    if (processId == 0) {
        LOG_ERROR(L"Invalid process ID: 0");
        return ErrorCode::INVALID_PARAMETER;
    }
    
    // 检查是否为系统关键进程
    if (SecurityUtils::IsSystemCriticalProcess(processId)) {
        LOG_ERROR(L"Cannot inject into system critical process: " << processId);
        return ErrorCode::ACCESS_DENIED;
    }
    
    // 验证DLL路径安全性
    if (!SecurityUtils::ValidateDllPath(dllPath)) {
        LOG_ERROR(L"DLL path failed security validation: " << dllPath);
        return ErrorCode::INVALID_PARAMETER;
    }
    
    if (!data.IsValid()) {
        LOG_ERROR(L"Invalid injection data");
        return ErrorCode::INVALID_PARAMETER;
    }
    
    // 检查当前进程权限
    if (!SecurityUtils::HasSufficientPrivileges()) {
        LOG_ERROR(L"Insufficient privileges for DLL injection");
        return ErrorCode::ACCESS_DENIED;
    }
    
    if (!FileExists(dllPath)) {
        LOG_ERROR(L"DLL file not found: " << dllPath);
        return ErrorCode::FILE_NOT_FOUND;
    }

    // 获取进程句柄
    ScopedHandle<HANDLE> hProcess(GetProcessHandle(processId));
    if (!hProcess) {
        DWORD error = GetLastError();
        LOG_ERROR(L"Failed to open target process " << processId << L", error: " << error);
        
        if (error == ERROR_ACCESS_DENIED) {
            LOG_ERROR(L"Access denied. Try running as administrator.");
        }
        return false;
    }

    bool success = false;
    LPVOID pDllPath = nullptr;
    LPVOID pInjectionData = nullptr;
    ScopedHandle<HANDLE> hThread;

    try {
        // 在目标进程中分配内存存储DLL路径
        SIZE_T dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
        pDllPath = AllocateMemoryInProcess(hProcess.get(), dllPathSize);
        if (!pDllPath) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to allocate memory for DLL path, error: " << error);
            throw std::runtime_error("Memory allocation failed");
        }

        // 写入DLL路径
        if (!WriteDataToProcess(hProcess.get(), pDllPath, dllPath.c_str(), dllPathSize)) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to write DLL path to target process, error: " << error);
            throw std::runtime_error("Memory write failed");
        }

        // 分配内存存储注入数据
        pInjectionData = AllocateMemoryInProcess(hProcess.get(), sizeof(InjectionData));
        if (!pInjectionData) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to allocate memory for injection data, error: " << error);
            throw std::runtime_error("Memory allocation failed");
        }

        // 写入注入数据
        if (!WriteDataToProcess(hProcess.get(), pInjectionData, &data, sizeof(InjectionData))) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to write injection data to target process, error: " << error);
            throw std::runtime_error("Memory write failed");
        }

        // 获取LoadLibraryW函数地址
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!hKernel32) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to get kernel32.dll handle, error: " << error);
            throw std::runtime_error("Failed to get kernel32 handle");
        }

        LPTHREAD_START_ROUTINE pLoadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
        if (!pLoadLibraryW) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to get LoadLibraryW address, error: " << error);
            throw std::runtime_error("Failed to get LoadLibraryW address");
        }

        // 创建远程线程加载DLL
        hThread = ScopedHandle<HANDLE>(CreateRemoteThreadInProcess(hProcess.get(), pLoadLibraryW, pDllPath));
        if (!hThread) {
            DWORD error = GetLastError();
            LOG_ERROR(L"Failed to create remote thread, error: " << error);
            throw std::runtime_error("Failed to create remote thread");
        }

        // 等待线程完成
        if (!WaitForRemoteThread(hThread.get())) {
            LOG_ERROR(L"Remote thread execution failed or timed out");
            throw std::runtime_error("Remote thread execution failed");
        }

        // 检查线程退出码
        DWORD exitCode;
        if (GetExitCodeThread(hThread.get(), &exitCode)) {
            if (exitCode == 0) {
                LOG_ERROR(L"LoadLibraryW failed in target process");
                throw std::runtime_error("LoadLibraryW failed");
            }
            LOG_INFO(L"DLL loaded successfully, module handle: 0x" << std::hex << exitCode);
        }

        LOG_INFO(L"DLL injection successful");
        success = true;

    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during injection: " << StringToWString(e.what()));
        success = false;
    }

    // 清理资源
    if (pDllPath) {
        if (!VirtualFreeEx(hProcess.get(), pDllPath, 0, MEM_RELEASE)) {
            LOG_WARNING(L"Failed to free DLL path memory in target process");
        }
    }
    if (pInjectionData) {
        if (!VirtualFreeEx(hProcess.get(), pInjectionData, 0, MEM_RELEASE)) {
            LOG_WARNING(L"Failed to free injection data memory in target process");
        }
    }

    return success;
}

bool DllInjector::EjectDll(DWORD processId, const std::wstring& dllPath) {
    LOG_INFO(L"Ejecting DLL from process " << processId);
    
    // 输入验证
    if (processId == 0) {
        LOG_ERROR(L"Invalid process ID: 0");
        return false;
    }
    
    if (dllPath.empty()) {
        LOG_ERROR(L"DLL path cannot be empty");
        return false;
    }
    
    // 获取进程句柄
    ScopedHandle<HANDLE> hProcess(GetProcessHandle(processId));
    if (!hProcess) {
        DWORD error = GetLastError();
        LOG_ERROR(L"Failed to open target process " << processId << L", error: " << error);
        return false;
    }
    
    try {
        // 获取FreeLibrary函数地址
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!hKernel32) {
            LOG_ERROR(L"Failed to get kernel32.dll handle");
            return false;
        }
        
        LPTHREAD_START_ROUTINE pFreeLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "FreeLibrary");
        if (!pFreeLibrary) {
            LOG_ERROR(L"Failed to get FreeLibrary address");
            return false;
        }
        
        // 这里需要找到目标进程中DLL的模块句柄
        // 简化实现，实际应该枚举目标进程的模块
        LOG_WARNING(L"DLL ejection not fully implemented - requires module enumeration");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during DLL ejection: " << StringToWString(e.what()));
        return false;
    }
}

HANDLE DllInjector::GetProcessHandle(DWORD processId) {
    if (processId == 0) {
        LOG_ERROR(L"Invalid process ID: 0");
        return nullptr;
    }
    
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
        FALSE, processId);
        
    if (!hProcess) {
        DWORD error = GetLastError();
        LOG_DEBUG(L"Failed to open process " << processId << L", error: " << error);
    }
    
    return hProcess;
}

LPVOID DllInjector::AllocateMemoryInProcess(HANDLE hProcess, SIZE_T size) {
    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"Invalid process handle");
        return nullptr;
    }
    
    if (size == 0) {
        LOG_ERROR(L"Invalid allocation size: 0");
        return nullptr;
    }
    
    LPVOID pMemory = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pMemory) {
        DWORD error = GetLastError();
        LOG_ERROR(L"VirtualAllocEx failed, size: " << size << L", error: " << error);
    }
    
    return pMemory;
}

bool DllInjector::WriteDataToProcess(HANDLE hProcess, LPVOID address, const void* data, SIZE_T size) {
    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"Invalid process handle");
        return false;
    }
    
    if (!address) {
        LOG_ERROR(L"Invalid memory address");
        return false;
    }
    
    if (!data) {
        LOG_ERROR(L"Invalid data pointer");
        return false;
    }
    
    if (size == 0) {
        LOG_ERROR(L"Invalid data size: 0");
        return false;
    }
    
    SIZE_T bytesWritten = 0;
    BOOL result = WriteProcessMemory(hProcess, address, data, size, &bytesWritten);
    
    if (!result) {
        DWORD error = GetLastError();
        LOG_ERROR(L"WriteProcessMemory failed, error: " << error);
        return false;
    }
    
    if (bytesWritten != size) {
        LOG_ERROR(L"Partial write: expected " << size << L" bytes, wrote " << bytesWritten);
        return false;
    }
    
    return true;
}

HANDLE DllInjector::CreateRemoteThreadInProcess(HANDLE hProcess, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter) {
    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"Invalid process handle");
        return nullptr;
    }
    
    if (!startAddress) {
        LOG_ERROR(L"Invalid thread start address");
        return nullptr;
    }
    
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, startAddress, parameter, 0, NULL);
    if (!hThread) {
        DWORD error = GetLastError();
        LOG_ERROR(L"CreateRemoteThread failed, error: " << error);
    }
    
    return hThread;
}

bool DllInjector::WaitForRemoteThread(HANDLE hThread, DWORD timeout) {
    if (!hThread || hThread == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"Invalid thread handle");
        return false;
    }
    
    DWORD waitResult = WaitForSingleObject(hThread, timeout);
    
    switch (waitResult) {
        case WAIT_OBJECT_0:
            LOG_DEBUG(L"Remote thread completed successfully");
            return true;
            
        case WAIT_TIMEOUT:
            LOG_ERROR(L"Remote thread timed out after " << timeout << L" ms");
            return false;
            
        case WAIT_FAILED:
            {
                DWORD error = GetLastError();
                LOG_ERROR(L"WaitForSingleObject failed, error: " << error);
                return false;
            }
            
        default:
            LOG_ERROR(L"Unexpected wait result: " << waitResult);
            return false;
    }
}