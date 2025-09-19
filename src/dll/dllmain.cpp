#include "../include/common.h"
#include "../include/jar_loader.h"
#include "../include/hot_reload.h"
#include <memory>

// 前向声明
DWORD InitializeJarInjection();
void CleanupJarInjection();

// 全局变量
static std::unique_ptr<JarLoader> g_jarLoader;
static std::unique_ptr<HotReloadManager> g_hotReloadManager;
static InjectionData g_injectionData;

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // DLL被加载时执行
        DisableThreadLibraryCalls(hModule);
        
        // 创建一个新线程来执行JAR加载，避免阻塞DLL加载过程
        CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
            return InitializeJarInjection();
        }, NULL, 0, NULL);
        
        break;
        
    case DLL_PROCESS_DETACH:
        // DLL被卸载时执行
        CleanupJarInjection();
        break;
        
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

// 初始化JAR注入
DWORD InitializeJarInjection() {
    try {
        LOG_INFO(L"Initializing JAR injection...");
        
        // 创建JAR加载器
        g_jarLoader = std::make_unique<JarLoader>();
        
        // 初始化JVM
        if (!g_jarLoader->InitializeJVM()) {
            LOG_ERROR(L"Failed to initialize JVM");
            return 1;
        }
        
        // 加载JAR文件
        std::wstring jarPath(g_injectionData.jarPath);
        if (!g_jarLoader->LoadJar(jarPath)) {
            LOG_ERROR(L"Failed to load JAR file: " << jarPath);
            return 1;
        }
        
        // 调用Java方法
        std::string className = WStringToString(g_injectionData.className);
        std::string methodName = WStringToString(g_injectionData.methodName);
        
        if (!g_jarLoader->CallJavaMethod(className, methodName)) {
            LOG_ERROR(L"Failed to call Java method: " << g_injectionData.className << L"." << g_injectionData.methodName);
            return 1;
        }
        
        // 如果启用热重载，启动监控
        if (g_injectionData.enableHotReload) {
            g_hotReloadManager = std::make_unique<HotReloadManager>(g_jarLoader.get());
            if (!g_hotReloadManager->StartMonitoring(jarPath, className, methodName)) {
                LOG_ERROR(L"Failed to start hot reload monitoring");
            } else {
                LOG_INFO(L"Hot reload monitoring started");
            }
        }
        
        LOG_INFO(L"JAR injection completed successfully");
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during JAR injection: " << StringToWString(e.what()));
        return 1;
    } catch (...) {
        LOG_ERROR(L"Unknown exception during JAR injection");
        return 1;
    }
}

// 清理JAR注入
void CleanupJarInjection() {
    try {
        LOG_INFO(L"Cleaning up JAR injection...");
        
        // 停止热重载监控
        if (g_hotReloadManager) {
            g_hotReloadManager->StopMonitoring();
            g_hotReloadManager.reset();
        }
        
        // 卸载JAR
        if (g_jarLoader) {
            g_jarLoader->UnloadJar();
            g_jarLoader.reset();
        }
        
        LOG_INFO(L"JAR injection cleanup completed");
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during cleanup: " << StringToWString(e.what()));
    } catch (...) {
        LOG_ERROR(L"Unknown exception during cleanup");
    }
}

// 导出函数：设置注入数据
extern "C" __declspec(dllexport) void SetInjectionData(const InjectionData* data) {
    if (data) {
        g_injectionData = *data;
        LOG_INFO(L"Injection data set: JAR=" << g_injectionData.jarPath << 
                 L", Class=" << g_injectionData.className << 
                 L", Method=" << g_injectionData.methodName);
    }
}

// 导出函数：获取注入状态
extern "C" __declspec(dllexport) bool GetInjectionStatus() {
    return g_jarLoader && g_jarLoader->IsInitialized();
}