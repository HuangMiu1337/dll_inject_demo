#pragma once

#include "common.h"
#include "security_utils.h"
#include <tlhelp32.h>
#include <psapi.h>

class DllInjector {
public:
    DllInjector();
    ~DllInjector();

    // 根据进程名查找进程ID
    std::vector<DWORD> FindProcessByName(const std::wstring& processName);
    
    // 注入DLL到指定进程
    bool InjectDll(DWORD processId, const std::wstring& dllPath, const InjectionData& data);
    
    // 卸载DLL
    bool EjectDll(DWORD processId, const std::wstring& dllPath);

private:
    // 获取进程句柄
    HANDLE GetProcessHandle(DWORD processId);
    
    // 在目标进程中分配内存
    LPVOID AllocateMemoryInProcess(HANDLE hProcess, SIZE_T size);
    
    // 写入数据到目标进程
    bool WriteDataToProcess(HANDLE hProcess, LPVOID address, const void* data, SIZE_T size);
    
    // 创建远程线程
    HANDLE CreateRemoteThreadInProcess(HANDLE hProcess, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter);
    
    // 等待远程线程完成
    bool WaitForRemoteThread(HANDLE hThread, DWORD timeout = INJECTION_TIMEOUT);
};