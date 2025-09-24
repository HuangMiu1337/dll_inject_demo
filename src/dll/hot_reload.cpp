#include "../include/hot_reload.h"
#include <future>
#include <chrono>

HotReloadManager::HotReloadManager(JarLoader* jarLoader) 
    : jarLoader_(jarLoader), monitoring_(false), lastError_(ErrorCode::SUCCESS) {
    if (!jarLoader_) {
        LOG_ERROR(L"JarLoader pointer is null in HotReloadManager constructor");
        lastError_ = ErrorCode::INVALID_PARAMETER;
    } else {
        LOG_DEBUG(L"HotReloadManager created successfully");
    }
}

HotReloadManager::~HotReloadManager() {
    LOG_DEBUG(L"HotReloadManager destroying...");
    StopMonitoring();
    LOG_DEBUG(L"HotReloadManager destroyed");
}

ErrorCode HotReloadManager::StartMonitoring(const std::wstring& jarPath, const std::string& className, const std::string& methodName) {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    
    if (monitoring_) {
        LOG_INFO(L"Hot reload monitoring already started");
        SetLastError(ErrorCode::SUCCESS);
        return ErrorCode::SUCCESS;
    }
    
    // 输入验证
    if (!jarLoader_) {
        LOG_ERROR(L"JarLoader is null");
        SetLastError(ErrorCode::INVALID_PARAMETER);
        return ErrorCode::INVALID_PARAMETER;
    }
    
    // 安全验证JAR路径
    if (!SecurityUtils::ValidateJarPath(jarPath)) {
        LOG_ERROR(L"JAR path failed security validation: " << jarPath);
        SetLastError(ErrorCode::INVALID_PARAMETER);
        return ErrorCode::INVALID_PARAMETER;
    }
    
    // 安全验证类名和方法名
    if (!SecurityUtils::ValidateClassName(className)) {
        LOG_ERROR(L"Class name failed security validation: " << StringToWString(className));
        SetLastError(ErrorCode::INVALID_PARAMETER);
        return ErrorCode::INVALID_PARAMETER;
    }
    
    if (!SecurityUtils::ValidateMethodName(methodName)) {
        LOG_ERROR(L"Method name failed security validation: " << StringToWString(methodName));
        SetLastError(ErrorCode::INVALID_PARAMETER);
        return ErrorCode::INVALID_PARAMETER;
    }
    
    // 验证输入长度
    if (className.length() > MAX_CLASS_NAME_LENGTH || methodName.length() > MAX_METHOD_NAME_LENGTH) {
        LOG_ERROR(L"Class name or method name too long");
        SetLastError(ErrorCode::INVALID_PARAMETER);
        return ErrorCode::INVALID_PARAMETER;
    }
    
    if (!FileExists(jarPath)) {
        LOG_ERROR(L"JAR file not found for monitoring: " << jarPath);
        SetLastError(ErrorCode::FILE_NOT_FOUND);
        return ErrorCode::FILE_NOT_FOUND;
    }
    
    try {
        // 保存监控参数
        watchedJarPath_ = jarPath;
        watchedClassName_ = className;
        watchedMethodName_ = methodName;
        lastModifyTime_ = GetFileModifyTime(jarPath);
        
        // 启动监控
        monitoring_ = true;
        
        monitorThread_ = std::thread(&HotReloadManager::MonitorThreadFunc, this);
        
        // 等待线程启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (!monitoring_) {
            LOG_ERROR(L"Monitor thread failed to start properly");
            SetLastError(ErrorCode::THREAD_CREATION_FAILED);
            return ErrorCode::THREAD_CREATION_FAILED;
        }
        
        LOG_INFO(L"Hot reload monitoring started for: " << jarPath);
        SetLastError(ErrorCode::SUCCESS);
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        monitoring_ = false;
        LOG_ERROR(L"Failed to start monitoring thread: " << StringToWString(e.what()));
        SetLastError(ErrorCode::THREAD_CREATION_FAILED);
        return ErrorCode::THREAD_CREATION_FAILED;
    } catch (...) {
        monitoring_ = false;
        LOG_ERROR(L"Unknown exception while starting monitoring thread");
        SetLastError(ErrorCode::UNKNOWN_ERROR);
        return ErrorCode::UNKNOWN_ERROR;
    }
}

void HotReloadManager::StopMonitoring() {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    
    if (!monitoring_) {
        LOG_DEBUG(L"Hot reload monitoring already stopped");
        return;
    }
    
    LOG_INFO(L"Stopping hot reload monitoring...");
    monitoring_ = false;
    
    if (monitorThread_.joinable()) {
        try {
            // 等待线程结束，最多等待5秒
            auto future = std::async(std::launch::async, [this]() {
                monitorThread_.join();
            });
            
            if (future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
                LOG_WARNING(L"Monitor thread did not stop within timeout, detaching...");
                monitorThread_.detach();
            } else {
                LOG_DEBUG(L"Monitor thread joined successfully");
            }
        } catch (const std::exception& e) {
            LOG_ERROR(L"Exception while stopping monitor thread: " << StringToWString(e.what()));
            if (monitorThread_.joinable()) {
                monitorThread_.detach();
            }
        }
    }
    
    // 清理监控状态
    watchedJarPath_.clear();
    watchedClassName_.clear();
    watchedMethodName_.clear();
    
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
        ErrorCode unloadResult = jarLoader_->UnloadJar();
        if (unloadResult != ErrorCode::SUCCESS) {
            LOG_ERROR(L"Failed to unload current JAR");
            return false;
        }
        
        // 等待一小段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 重新加载JAR
        ErrorCode loadResult = jarLoader_->LoadJar(watchedJarPath_);
        if (loadResult != ErrorCode::SUCCESS) {
            LOG_ERROR(L"Failed to reload JAR");
            return false;
        }
        
        // 调用指定的方法
        if (!watchedClassName_.empty() && !watchedMethodName_.empty()) {
            ErrorCode result = jarLoader_->CallJavaMethod(watchedClassName_, watchedMethodName_);
            if (result != ErrorCode::SUCCESS) {
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

void HotReloadManager::SetLastError(ErrorCode error) {
    lastError_ = error;
}

ErrorCode HotReloadManager::GetLastError() const {
    return lastError_;
}