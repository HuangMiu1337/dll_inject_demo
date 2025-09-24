#pragma once

#include "common.h"
#include "jar_loader.h"
#include "security_utils.h"
#include <mutex>

class HotReloadManager {
public:
    HotReloadManager(JarLoader* jarLoader);
    ~HotReloadManager();

    // 开始监控文件变化
    ErrorCode StartMonitoring(const std::wstring& jarPath, const std::string& className, const std::string& methodName);
    
    // 停止监控
    void StopMonitoring();
    
    // 检查是否正在监控
    bool IsMonitoring() const { return monitoring_; }
    
    // 获取最后的错误
    ErrorCode GetLastError() const { return lastError_; }

private:
    JarLoader* jarLoader_;
    std::atomic<bool> monitoring_;
    std::thread monitorThread_;
    std::wstring watchedJarPath_;
    std::string watchedClassName_;
    std::string watchedMethodName_;
    FILETIME lastModifyTime_;
    ErrorCode lastError_;
    mutable std::mutex monitorMutex_;
    
    // 监控线程函数
    void MonitorThreadFunc();
    
    // 检查文件是否被修改
    bool IsFileModified();
    
    // 重新加载JAR
    bool ReloadJar();
    
    // 设置错误码
    void SetLastError(ErrorCode error) { lastError_ = error; }
};