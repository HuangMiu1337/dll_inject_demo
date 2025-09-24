#include "../include/common.h"
#include <iomanip>
#include <sstream>

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::SetLogFile(const std::wstring& filePath) {
    std::lock_guard<std::mutex> lock(logMutex_);
    logFilePath_ = filePath;
}

void Logger::Log(LogLevel level, const std::wstring& message, const char* file, int line) {
    if (level < logLevel_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::wstringstream logEntry;
    logEntry << GetTimestamp() << L" [" << LogLevelToString(level) << L"] ";
    
    if (file && line > 0) {
        std::string fileName = file;
        size_t lastSlash = fileName.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            fileName = fileName.substr(lastSlash + 1);
        }
        logEntry << L"(" << StringToWString(fileName) << L":" << line << L") ";
    }
    
    logEntry << message;
    
    // 输出到控制台
    std::wcout << logEntry.str() << std::endl;
    
    // 输出到文件（如果设置了日志文件）
    if (!logFilePath_.empty()) {
        std::wofstream logFile(logFilePath_, std::ios::app);
        if (logFile.is_open()) {
            logFile << logEntry.str() << std::endl;
            logFile.close();
        }
    }
}

std::wstring Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::wstringstream ss;
    ss << std::put_time(std::localtime(&time_t), L"%Y-%m-%d %H:%M:%S");
    ss << L"." << std::setfill(L'0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::wstring Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return L"DEBUG";
        case LogLevel::INFO: return L"INFO";
        case LogLevel::WARNING: return L"WARN";
        case LogLevel::ERROR: return L"ERROR";
        case LogLevel::CRITICAL: return L"CRITICAL";
        default: return L"UNKNOWN";
    }
}