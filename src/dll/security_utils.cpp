#include "../include/security_utils.h"
#include <shlwapi.h>
#include <wintrust.h>
#include <softpub.h>
#include <random>
#include <sstream>
#include <iomanip>

// 静态成员初始化
const std::string SecurityUtils::DANGEROUS_CHARS = ";<>|&$`\"'\\*?[]{}()";
const std::wstring SecurityUtils::DANGEROUS_WCHARS = L";<>|&$`\"'\\*?[]{}()";

const std::vector<std::wstring> SecurityUtils::SYSTEM_CRITICAL_PROCESSES = {
    L"csrss.exe", L"winlogon.exe", L"services.exe", L"lsass.exe", L"svchost.exe",
    L"explorer.exe", L"dwm.exe", L"wininit.exe", L"smss.exe", L"system",
    L"registry", L"audiodg.exe", L"conhost.exe", L"dllhost.exe"
};

const std::vector<std::wstring> SecurityUtils::ALLOWED_DLL_EXTENSIONS = {
    L".dll", L".DLL"
};

const std::vector<std::wstring> SecurityUtils::ALLOWED_JAR_EXTENSIONS = {
    L".jar", L".JAR"
};

const std::vector<std::wstring> SecurityUtils::FORBIDDEN_PATH_PATTERNS = {
    L"\\Windows\\System32\\", L"\\Windows\\SysWOW64\\", L"\\Windows\\WinSxS\\",
    L"\\Program Files\\Windows Defender\\", L"\\Program Files (x86)\\Windows Defender\\",
    L"\\Windows\\Boot\\", L"\\Windows\\Fonts\\", L"\\Windows\\inf\\",
    L"\\$Recycle.Bin\\", L"\\System Volume Information\\"
};

bool SecurityUtils::ValidateProcessName(const std::wstring& processName) {
    if (processName.empty() || processName.length() > 260) {
        LOG_ERROR(L"Invalid process name length");
        return false;
    }
    
    // 检查危险字符
    if (ContainsDangerousChars(processName)) {
        LOG_ERROR(L"Process name contains dangerous characters");
        return false;
    }
    
    // 检查是否为系统关键进程
    if (IsSystemCriticalProcess(processName)) {
        LOG_WARNING(L"Attempting to target system critical process: " << processName);
        return false;
    }
    
    // 检查是否包含路径分隔符（进程名不应包含路径）
    if (processName.find(L'\\') != std::wstring::npos || processName.find(L'/') != std::wstring::npos) {
        LOG_ERROR(L"Process name should not contain path separators");
        return false;
    }
    
    return true;
}

bool SecurityUtils::ValidateDllPath(const std::wstring& dllPath) {
    if (dllPath.empty() || dllPath.length() > MAX_PATH) {
        LOG_ERROR(L"Invalid DLL path length");
        return false;
    }
    
    // 检查文件扩展名
    if (!HasValidExtension(dllPath, ALLOWED_DLL_EXTENSIONS)) {
        LOG_ERROR(L"Invalid DLL file extension");
        return false;
    }
    
    // 检查路径安全性
    if (!IsPathSafe(dllPath)) {
        LOG_ERROR(L"DLL path is not safe");
        return false;
    }
    
    // 检查文件是否存在
    if (!FileExists(dllPath)) {
        LOG_ERROR(L"DLL file does not exist: " << dllPath);
        return false;
    }
    
    return true;
}

bool SecurityUtils::ValidateJarPath(const std::wstring& jarPath) {
    if (jarPath.empty() || jarPath.length() > MAX_PATH) {
        LOG_ERROR(L"Invalid JAR path length");
        return false;
    }
    
    // 检查文件扩展名
    if (!HasValidExtension(jarPath, ALLOWED_JAR_EXTENSIONS)) {
        LOG_ERROR(L"Invalid JAR file extension");
        return false;
    }
    
    // 检查路径安全性
    if (!IsPathSafe(jarPath)) {
        LOG_ERROR(L"JAR path is not safe");
        return false;
    }
    
    // 检查文件是否存在
    if (!FileExists(jarPath)) {
        LOG_ERROR(L"JAR file does not exist: " << jarPath);
        return false;
    }
    
    return true;
}

bool SecurityUtils::ValidateClassName(const std::string& className) {
    if (className.empty() || className.length() > MAX_CLASS_NAME_LENGTH) {
        LOG_ERROR(L"Invalid class name length");
        return false;
    }
    
    // 检查危险字符
    if (ContainsDangerousChars(className)) {
        LOG_ERROR(L"Class name contains dangerous characters");
        return false;
    }
    
    // 检查Java类名格式（简单验证）
    std::regex classNamePattern(R"(^[a-zA-Z_$][a-zA-Z0-9_$]*(\.[a-zA-Z_$][a-zA-Z0-9_$]*)*$)");
    if (!std::regex_match(className, classNamePattern)) {
        LOG_ERROR(L"Invalid Java class name format");
        return false;
    }
    
    return true;
}

bool SecurityUtils::ValidateMethodName(const std::string& methodName) {
    if (methodName.empty() || methodName.length() > MAX_METHOD_NAME_LENGTH) {
        LOG_ERROR(L"Invalid method name length");
        return false;
    }
    
    // 检查危险字符
    if (ContainsDangerousChars(methodName)) {
        LOG_ERROR(L"Method name contains dangerous characters");
        return false;
    }
    
    // 检查Java方法名格式（简单验证）
    std::regex methodNamePattern(R"(^[a-zA-Z_$][a-zA-Z0-9_$]*$)");
    if (!std::regex_match(methodName, methodNamePattern)) {
        LOG_ERROR(L"Invalid Java method name format");
        return false;
    }
    
    return true;
}

bool SecurityUtils::ContainsDangerousChars(const std::string& input) {
    return input.find_first_of(DANGEROUS_CHARS) != std::string::npos;
}

bool SecurityUtils::ContainsDangerousChars(const std::wstring& input) {
    return input.find_first_of(DANGEROUS_WCHARS) != std::wstring::npos;
}

bool SecurityUtils::IsPathSafe(const std::wstring& path) {
    // 规范化路径
    std::wstring normalizedPath = NormalizePath(path);
    
    // 检查是否匹配禁止模式
    if (MatchesForbiddenPattern(normalizedPath)) {
        LOG_ERROR(L"Path matches forbidden pattern: " << normalizedPath);
        return false;
    }
    
    // 检查是否包含相对路径元素
    if (normalizedPath.find(L"..") != std::wstring::npos) {
        LOG_ERROR(L"Path contains relative path elements");
        return false;
    }
    
    // 检查是否为绝对路径
    if (normalizedPath.length() < 3 || normalizedPath[1] != L':' || normalizedPath[2] != L'\\') {
        LOG_ERROR(L"Path is not an absolute Windows path");
        return false;
    }
    
    return true;
}

bool SecurityUtils::HasSufficientPrivileges() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        LOG_ERROR(L"Failed to open process token, error: " << GetLastError());
        return false;
    }
    
    ScopedHandle tokenHandle(hToken);
    
    TOKEN_ELEVATION elevation;
    DWORD dwSize;
    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
        LOG_ERROR(L"Failed to get token elevation info, error: " << GetLastError());
        return false;
    }
    
    if (!elevation.TokenIsElevated) {
        LOG_WARNING(L"Process is not running with elevated privileges");
        return false;
    }
    
    return true;
}

bool SecurityUtils::IsSystemCriticalProcess(DWORD processId) {
    // 系统进程通常有较低的PID
    if (processId <= 8) {
        return true;
    }
    
    ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId));
    if (!hProcess.IsValid()) {
        // 如果无法打开进程，可能是系统进程
        return true;
    }
    
    wchar_t processName[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(hProcess.Get(), 0, processName, &size)) {
        std::wstring fullPath(processName);
        size_t lastSlash = fullPath.find_last_of(L'\\');
        if (lastSlash != std::wstring::npos) {
            std::wstring fileName = fullPath.substr(lastSlash + 1);
            return IsSystemCriticalProcess(fileName);
        }
    }
    
    return false;
}

bool SecurityUtils::IsSystemCriticalProcess(const std::wstring& processName) {
    std::wstring lowerProcessName = processName;
    std::transform(lowerProcessName.begin(), lowerProcessName.end(), lowerProcessName.begin(), ::towlower);
    
    for (const auto& criticalProcess : SYSTEM_CRITICAL_PROCESSES) {
        std::wstring lowerCritical = criticalProcess;
        std::transform(lowerCritical.begin(), lowerCritical.end(), lowerCritical.begin(), ::towlower);
        
        if (lowerProcessName == lowerCritical) {
            return true;
        }
    }
    
    return false;
}

bool SecurityUtils::VerifyFileSignature(const std::wstring& filePath) {
    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = filePath.c_str();
    fileInfo.hFile = NULL;
    fileInfo.pgKnownSubject = NULL;
    
    WINTRUST_DATA winTrustData = {};
    winTrustData.cbStruct = sizeof(WINTRUST_DATA);
    winTrustData.pPolicyCallbackData = NULL;
    winTrustData.pSIPClientData = NULL;
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    winTrustData.hWVTStateData = NULL;
    winTrustData.pwszURLReference = NULL;
    winTrustData.dwProvFlags = WTD_SAFER_FLAG;
    winTrustData.pFile = &fileInfo;
    
    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG result = WinVerifyTrust(NULL, &policyGUID, &winTrustData);
    
    // 清理
    winTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &policyGUID, &winTrustData);
    
    return result == ERROR_SUCCESS;
}

void SecurityUtils::SecureZeroMemory(void* ptr, size_t size) {
    if (ptr && size > 0) {
        volatile char* vptr = static_cast<volatile char*>(ptr);
        for (size_t i = 0; i < size; ++i) {
            vptr[i] = 0;
        }
    }
}

std::wstring SecurityUtils::GenerateSecureTempFileName() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::wstringstream ss;
    ss << L"temp_";
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }
    ss << L".tmp";
    
    return ss.str();
}

bool SecurityUtils::HasValidExtension(const std::wstring& filePath, const std::vector<std::wstring>& allowedExtensions) {
    size_t dotPos = filePath.find_last_of(L'.');
    if (dotPos == std::wstring::npos) {
        return false;
    }
    
    std::wstring extension = filePath.substr(dotPos);
    for (const auto& allowedExt : allowedExtensions) {
        if (_wcsicmp(extension.c_str(), allowedExt.c_str()) == 0) {
            return true;
        }
    }
    
    return false;
}

bool SecurityUtils::MatchesForbiddenPattern(const std::wstring& path) {
    std::wstring upperPath = path;
    std::transform(upperPath.begin(), upperPath.end(), upperPath.begin(), ::towupper);
    
    for (const auto& pattern : FORBIDDEN_PATH_PATTERNS) {
        std::wstring upperPattern = pattern;
        std::transform(upperPattern.begin(), upperPattern.end(), upperPattern.begin(), ::towupper);
        
        if (upperPath.find(upperPattern) != std::wstring::npos) {
            return true;
        }
    }
    
    return false;
}

std::wstring SecurityUtils::NormalizePath(const std::wstring& path) {
    wchar_t buffer[MAX_PATH];
    if (GetFullPathNameW(path.c_str(), MAX_PATH, buffer, NULL) == 0) {
        return path; // 如果失败，返回原路径
    }
    
    return std::wstring(buffer);
}