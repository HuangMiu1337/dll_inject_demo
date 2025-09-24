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
#include <chrono>
#include <sstream>

// 错误码定义
enum class ErrorCode : int {
    SUCCESS = 0,
    PROCESS_NOT_FOUND = 1001,
    PROCESS_ACCESS_DENIED = 1002,
    MEMORY_ALLOCATION_FAILED = 1003,
    MEMORY_WRITE_FAILED = 1004,
    THREAD_CREATION_FAILED = 1005,
    THREAD_TIMEOUT = 1006,
    DLL_NOT_FOUND = 1007,
    JVM_INIT_FAILED = 2001,
    JAR_NOT_FOUND = 2002,
    JAR_LOAD_FAILED = 2003,
    JAVA_CLASS_NOT_FOUND = 2004,
    JAVA_METHOD_NOT_FOUND = 2005,
    JAVA_METHOD_CALL_FAILED = 2006,
    HOT_RELOAD_START_FAILED = 3001,
    FILE_MONITOR_FAILED = 3002,
    INVALID_PARAMETER = 4001,
    SECURITY_CHECK_FAILED = 4002,
    UNKNOWN_ERROR = 9999
};

// 日志级别
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

// 日志管理器
class Logger {
public:
    static Logger& GetInstance();
    void SetLogLevel(LogLevel level) { logLevel_ = level; }
    void SetLogFile(const std::wstring& filePath);
    void Log(LogLevel level, const std::wstring& message, const char* file = nullptr, int line = 0);
    
private:
    Logger() : logLevel_(LogLevel::INFO) {}
    LogLevel logLevel_;
    std::wstring logFilePath_;
    std::mutex logMutex_;
    
    std::wstring GetTimestamp();
    std::wstring LogLevelToString(LogLevel level);
};

// 改进的日志宏
#define LOG_DEBUG(msg) do { \
    std::wstringstream ss; \
    ss << msg; \
    Logger::GetInstance().Log(LogLevel::DEBUG, ss.str(), __FILE__, __LINE__); \
} while(0)

#define LOG_INFO(msg) do { \
    std::wstringstream ss; \
    ss << msg; \
    Logger::GetInstance().Log(LogLevel::INFO, ss.str(), __FILE__, __LINE__); \
} while(0)

#define LOG_WARNING(msg) do { \
    std::wstringstream ss; \
    ss << msg; \
    Logger::GetInstance().Log(LogLevel::WARNING, ss.str(), __FILE__, __LINE__); \
} while(0)

#define LOG_ERROR(msg) do { \
    std::wstringstream ss; \
    ss << msg; \
    Logger::GetInstance().Log(LogLevel::ERROR, ss.str(), __FILE__, __LINE__); \
} while(0)

#define LOG_CRITICAL(msg) do { \
    std::wstringstream ss; \
    ss << msg; \
    Logger::GetInstance().Log(LogLevel::CRITICAL, ss.str(), __FILE__, __LINE__); \
} while(0)

// 常量定义
constexpr DWORD INJECTION_TIMEOUT = 5000; // 5秒超时
constexpr DWORD HOT_RELOAD_CHECK_INTERVAL = 1000; // 1秒检查间隔
constexpr size_t MAX_CLASS_NAME_LENGTH = 256;
constexpr size_t MAX_METHOD_NAME_LENGTH = 128;

// 结构体定义
struct InjectionData {
    wchar_t jarPath[MAX_PATH];
    wchar_t className[MAX_CLASS_NAME_LENGTH];
    wchar_t methodName[MAX_METHOD_NAME_LENGTH];
    bool enableHotReload;
    ErrorCode lastError;
    DWORD processId;
    DWORD threadId;
    
    // 构造函数
    InjectionData() : enableHotReload(false), lastError(ErrorCode::SUCCESS), processId(0), threadId(0) {
        memset(jarPath, 0, sizeof(jarPath));
        memset(className, 0, sizeof(className));
        memset(methodName, 0, sizeof(methodName));
    }
    
    // 验证数据有效性
    bool IsValid() const {
        return wcslen(jarPath) > 0 && 
               wcslen(className) > 0 && 
               wcslen(methodName) > 0 &&
               wcslen(jarPath) < MAX_PATH &&
               wcslen(className) < MAX_CLASS_NAME_LENGTH &&
               wcslen(methodName) < MAX_METHOD_NAME_LENGTH;
    }
};

// 异常安全的资源管理器
template<typename T>
class ScopedHandle {
public:
    explicit ScopedHandle(T handle = nullptr) : handle_(handle) {}
    
    ~ScopedHandle() {
        if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
    }
    
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
    
    ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
                CloseHandle(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    T get() const { return handle_; }
    T* operator&() { return &handle_; }
    operator bool() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }
    
private:
    T handle_;
};

// 工具函数
std::wstring StringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);
bool FileExists(const std::wstring& path);
FILETIME GetFileModifyTime(const std::wstring& path);