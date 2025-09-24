#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <regex>

class SecurityUtils {
public:
    // 验证进程名是否安全
    static bool ValidateProcessName(const std::wstring& processName);
    
    // 验证DLL路径是否安全
    static bool ValidateDllPath(const std::wstring& dllPath);
    
    // 验证JAR路径是否安全
    static bool ValidateJarPath(const std::wstring& jarPath);
    
    // 验证Java类名是否安全
    static bool ValidateClassName(const std::string& className);
    
    // 验证Java方法名是否安全
    static bool ValidateMethodName(const std::string& methodName);
    
    // 验证字符串是否包含危险字符
    static bool ContainsDangerousChars(const std::string& input);
    static bool ContainsDangerousChars(const std::wstring& input);
    
    // 检查路径是否在允许的目录内
    static bool IsPathSafe(const std::wstring& path);
    
    // 检查进程是否有足够的权限
    static bool HasSufficientPrivileges();
    
    // 检查目标进程是否为系统关键进程
    static bool IsSystemCriticalProcess(DWORD processId);
    static bool IsSystemCriticalProcess(const std::wstring& processName);
    
    // 验证文件签名（如果需要）
    static bool VerifyFileSignature(const std::wstring& filePath);
    
    // 清理敏感数据
    static void SecureZeroMemory(void* ptr, size_t size);
    
    // 生成安全的临时文件名
    static std::wstring GenerateSecureTempFileName();

private:
    // 危险字符列表
    static const std::string DANGEROUS_CHARS;
    static const std::wstring DANGEROUS_WCHARS;
    
    // 系统关键进程列表
    static const std::vector<std::wstring> SYSTEM_CRITICAL_PROCESSES;
    
    // 允许的文件扩展名
    static const std::vector<std::wstring> ALLOWED_DLL_EXTENSIONS;
    static const std::vector<std::wstring> ALLOWED_JAR_EXTENSIONS;
    
    // 禁止的路径模式
    static const std::vector<std::wstring> FORBIDDEN_PATH_PATTERNS;
    
    // 检查文件扩展名
    static bool HasValidExtension(const std::wstring& filePath, const std::vector<std::wstring>& allowedExtensions);
    
    // 检查路径是否匹配禁止模式
    static bool MatchesForbiddenPattern(const std::wstring& path);
    
    // 规范化路径
    static std::wstring NormalizePath(const std::wstring& path);
};