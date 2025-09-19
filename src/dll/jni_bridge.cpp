#include "../include/jar_loader.h"
#include "../include/common.h"

// JNI桥接函数，允许Java代码调用C++函数

extern "C" {

// 导出给Java调用的函数：记录日志
JNIEXPORT void JNICALL Java_NativeBridge_logMessage(JNIEnv* env, jclass clazz, jstring message) {
    const char* msg = env->GetStringUTFChars(message, nullptr);
    if (msg) {
        LOG_INFO(L"Java Log: " << StringToWString(msg));
        env->ReleaseStringUTFChars(message, msg);
    }
}

// 导出给Java调用的函数：获取系统信息
JNIEXPORT jstring JNICALL Java_NativeBridge_getSystemInfo(JNIEnv* env, jclass clazz) {
    std::string systemInfo = "Windows DLL Injection Demo - System Info:\n";
    
    // 获取系统信息
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    systemInfo += "Processor Architecture: " + std::to_string(sysInfo.wProcessorArchitecture) + "\n";
    systemInfo += "Number of Processors: " + std::to_string(sysInfo.dwNumberOfProcessors) + "\n";
    systemInfo += "Page Size: " + std::to_string(sysInfo.dwPageSize) + "\n";
    
    // 获取内存信息
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        systemInfo += "Total Physical Memory: " + std::to_string(memInfo.ullTotalPhys / (1024 * 1024)) + " MB\n";
        systemInfo += "Available Physical Memory: " + std::to_string(memInfo.ullAvailPhys / (1024 * 1024)) + " MB\n";
    }
    
    return env->NewStringUTF(systemInfo.c_str());
}

// 导出给Java调用的函数：执行系统命令
JNIEXPORT jstring JNICALL Java_NativeBridge_executeCommand(JNIEnv* env, jclass clazz, jstring command) {
    const char* cmd = env->GetStringUTFChars(command, nullptr);
    std::string result = "Command execution not implemented for security reasons";
    
    if (cmd) {
        LOG_INFO(L"Java requested command execution: " << StringToWString(cmd));
        env->ReleaseStringUTFChars(command, cmd);
    }
    
    return env->NewStringUTF(result.c_str());
}

// 导出给Java调用的函数：文件操作
JNIEXPORT jboolean JNICALL Java_NativeBridge_fileExists(JNIEnv* env, jclass clazz, jstring filePath) {
    const char* path = env->GetStringUTFChars(filePath, nullptr);
    bool exists = false;
    
    if (path) {
        std::wstring wPath = StringToWString(path);
        exists = FileExists(wPath);
        env->ReleaseStringUTFChars(filePath, path);
    }
    
    return exists ? JNI_TRUE : JNI_FALSE;
}

// 导出给Java调用的函数：获取当前进程ID
JNIEXPORT jint JNICALL Java_NativeBridge_getCurrentProcessId(JNIEnv* env, jclass clazz) {
    return static_cast<jint>(GetCurrentProcessId());
}

// 导出给Java调用的函数：获取当前线程ID
JNIEXPORT jint JNICALL Java_NativeBridge_getCurrentThreadId(JNIEnv* env, jclass clazz) {
    return static_cast<jint>(GetCurrentThreadId());
}

} // extern "C"

// 注册本地方法的辅助函数
bool RegisterNativeMethods(JNIEnv* env) {
    try {
        // 查找NativeBridge类
        jclass nativeBridgeClass = env->FindClass("NativeBridge");
        if (!nativeBridgeClass) {
            LOG_ERROR(L"Failed to find NativeBridge class");
            return false;
        }
        
        // 定义本地方法
        JNINativeMethod methods[] = {
            {"logMessage", "(Ljava/lang/String;)V", (void*)Java_NativeBridge_logMessage},
            {"getSystemInfo", "()Ljava/lang/String;", (void*)Java_NativeBridge_getSystemInfo},
            {"executeCommand", "(Ljava/lang/String;)Ljava/lang/String;", (void*)Java_NativeBridge_executeCommand},
            {"fileExists", "(Ljava/lang/String;)Z", (void*)Java_NativeBridge_fileExists},
            {"getCurrentProcessId", "()I", (void*)Java_NativeBridge_getCurrentProcessId},
            {"getCurrentThreadId", "()I", (void*)Java_NativeBridge_getCurrentThreadId}
        };
        
        // 注册本地方法
        int numMethods = sizeof(methods) / sizeof(methods[0]);
        if (env->RegisterNatives(nativeBridgeClass, methods, numMethods) != JNI_OK) {
            LOG_ERROR(L"Failed to register native methods");
            return false;
        }
        
        LOG_INFO(L"Native methods registered successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during native method registration: " << StringToWString(e.what()));
        return false;
    }
}

// 取消注册本地方法
void UnregisterNativeMethods(JNIEnv* env) {
    try {
        jclass nativeBridgeClass = env->FindClass("NativeBridge");
        if (nativeBridgeClass) {
            env->UnregisterNatives(nativeBridgeClass);
            LOG_INFO(L"Native methods unregistered");
        }
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during native method unregistration: " << StringToWString(e.what()));
    }
}