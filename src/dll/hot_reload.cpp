#include "../include/hot_reload.h"

HotReloadManager::HotReloadManager(JarLoader* jarLoader) 
    : jarLoader_(jarLoader), monitoring_(false) {
}

HotReloadManager::~HotReloadManager() {
    StopMonitoring();
}

bool HotReloadManager::StartMonitoring(const std::wstring& jarPath, const std::string& className, const std::string& methodName) {
    if (monitoring_) {
        LOG_INFO(L"Hot reload monitoring already started");
        return true;
    }
    
    if (!jarLoader_) {
        LOG_ERROR(L"JarLoader is null");
        return false;
    }
    
    if (!FileExists(jarPath)) {
        LOG_ERROR(L"JAR file not found for monitoring: " << jarPath);
        return false;
    }
    
    // 保存监控参数
    watchedJarPath_ = jarPath;
    watchedClassName_ = className;
    watchedMethodName_ = methodName;
    lastModifyTime_ = GetFileModifyTime(jarPath);
    
    // 启动监控
    monitoring_ = true;
    
    try {
        monitorThread_ = std::thread(&HotReloadManager::MonitorThreadFunc, this);
        LOG_INFO(L"Hot reload monitoring started for: " << jarPath);
        return true;
        
    } catch (const std::exception& e) {
        monitoring_ = false;
        LOG_ERROR(L"Failed to start monitoring thread: " << StringToWString(e.what()));
        return false;
    }
}

void HotReloadManager::StopMonitoring() {
    if (!monitoring_) {
        return;
    }
    
    LOG_INFO(L"Stopping hot reload monitoring...");
    monitoring_ = false;
    
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
    
    LOG_INFO(L"Hot reload monitoring stopped");
}

void HotReloadManager::MonitorThreadFunc() {
    LOG_INFO(L"Hot reload monitor thread started");
    
    while (monitoring_) {
        try {
            // 检查文件是否被修改
            if (IsFileModified()) {
                LOG_INFO(L"JAR file modification detected: " << watchedJarPath_);
                
                // 等待一小段时间确保文件写入完成
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // 重新加载JAR
                if (ReloadJar()) {
                    LOG_INFO(L"JAR hot reload successful");
                } else {
                    LOG_ERROR(L"JAR hot reload failed");
                }
                
                // 更新最后修改时间
                lastModifyTime_ = GetFileModifyTime(watchedJarPath_);
            }
            
            // 等待检查间隔
            std::this_thread::sleep_for(std::chrono::milliseconds(HOT_RELOAD_CHECK_INTERVAL));
            
        } catch (const std::exception& e) {
            LOG_ERROR(L"Exception in monitor thread: " << StringToWString(e.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(HOT_RELOAD_CHECK_INTERVAL));
        }
    }
    
    LOG_INFO(L"Hot reload monitor thread ended");
}

bool HotReloadManager::IsFileModified() {
    if (!FileExists(watchedJarPath_)) {
        LOG_ERROR(L"Watched JAR file no longer exists: " << watchedJarPath_);
        return false;
    }
    
    FILETIME currentModifyTime = GetFileModifyTime(watchedJarPath_);
    
    // 比较文件时间
    return CompareFileTime(&currentModifyTime, &lastModifyTime_) > 0;
}

bool HotReloadManager::ReloadJar() {
    if (!jarLoader_) {
        LOG_ERROR(L"JarLoader is null during reload");
        return false;
    }
    
    try {
        LOG_INFO(L"Reloading JAR: " << watchedJarPath_);
        
        // 卸载当前JAR
        if (!jarLoader_->UnloadJar()) {
            LOG_ERROR(L"Failed to unload current JAR");
            return false;
        }
        
        // 等待一小段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 重新加载JAR
        if (!jarLoader_->LoadJar(watchedJarPath_)) {
            LOG_ERROR(L"Failed to reload JAR");
            return false;
        }
        
        // 调用指定的方法
        if (!watchedClassName_.empty() && !watchedMethodName_.empty()) {
            if (!jarLoader_->CallJavaMethod(watchedClassName_, watchedMethodName_)) {
                LOG_ERROR(L"Failed to call method after reload: " << StringToWString(watchedClassName_ + "." + watchedMethodName_));
                return false;
            }
        }
        
        LOG_INFO(L"JAR reload completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during JAR reload: " << StringToWString(e.what()));
        return false;
    }
}

// 前向声明
int CompareFileTime(const FILETIME* ft1, const FILETIME* ft2);

// 辅助函数：比较两个FILETIME结构
int CompareFileTime(const FILETIME* ft1, const FILETIME* ft2) {
    ULARGE_INTEGER ul1, ul2;
    ul1.LowPart = ft1->dwLowDateTime;
    ul1.HighPart = ft1->dwHighDateTime;
    ul2.LowPart = ft2->dwLowDateTime;
    ul2.HighPart = ft2->dwHighDateTime;
    
    if (ul1.QuadPart < ul2.QuadPart) return -1;
    if (ul1.QuadPart > ul2.QuadPart) return 1;
    return 0;
}