#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>

// 日志宏
#define LOG_INFO(msg) std::wcout << L"[INFO] " << msg << std::endl
#define LOG_ERROR(msg) std::wcout << L"[ERROR] " << msg << std::endl
#define LOG_DEBUG(msg) std::wcout << L"[DEBUG] " << msg << std::endl

// 常量定义
constexpr DWORD INJECTION_TIMEOUT = 5000; // 5秒超时
constexpr DWORD HOT_RELOAD_CHECK_INTERVAL = 1000; // 1秒检查间隔

// 结构体定义
struct InjectionData {
    wchar_t jarPath[MAX_PATH];
    wchar_t className[256];
    wchar_t methodName[128];
    bool enableHotReload;
};

// 工具函数
std::wstring StringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);
bool FileExists(const std::wstring& path);
FILETIME GetFileModifyTime(const std::wstring& path);