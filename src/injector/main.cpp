#include "../include/dll_injector.h"
#include <iostream>
#include <filesystem>

void PrintUsage() {
    std::wcout << L"Usage: injector.exe <jar_path> [class_name] [method_name] [enable_hot_reload]" << std::endl;
    std::wcout << L"  jar_path: Path to the JAR file to inject" << std::endl;
    std::wcout << L"  class_name: Java class name (default: Main)" << std::endl;
    std::wcout << L"  method_name: Java method name (default: main)" << std::endl;
    std::wcout << L"  enable_hot_reload: Enable hot reload (default: true)" << std::endl;
    std::wcout << L"Example: injector.exe test.jar com.example.Main main true" << std::endl;
}

int wmain(int argc, wchar_t* argv[]) {
    std::wcout << L"DLL Injection Demo - JAR Injector" << std::endl;
    std::wcout << L"=================================" << std::endl;

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    // 解析命令行参数
    std::wstring jarPath = argv[1];
    std::wstring className = argc > 2 ? argv[2] : L"Main";
    std::wstring methodName = argc > 3 ? argv[3] : L"main";
    bool enableHotReload = argc > 4 ? (_wcsicmp(argv[4], L"true") == 0) : true;

    // 检查JAR文件是否存在
    if (!FileExists(jarPath)) {
        LOG_ERROR(L"JAR file not found: " << jarPath);
        return 1;
    }

    // 获取当前目录下的DLL路径
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    std::wstring dllPath = std::wstring(currentDir) + L"\\inject.dll";

    if (!FileExists(dllPath)) {
        LOG_ERROR(L"Injection DLL not found: " << dllPath);
        return 1;
    }

    // 创建注入数据
    InjectionData injectionData = {};
    wcscpy_s(injectionData.jarPath, jarPath.c_str());
    wcscpy_s(injectionData.className, className.c_str());
    wcscpy_s(injectionData.methodName, methodName.c_str());
    injectionData.enableHotReload = enableHotReload;

    // 创建DLL注入器
    DllInjector injector;

    // 查找javaw.exe进程
    std::vector<DWORD> processIds = injector.FindProcessByName(L"javaw.exe");
    if (processIds.empty()) {
        LOG_ERROR(L"No javaw.exe processes found. Please start a Java application first.");
        return 1;
    }

    // 如果找到多个进程，让用户选择
    DWORD targetProcessId;
    if (processIds.size() == 1) {
        targetProcessId = processIds[0];
        LOG_INFO(L"Found single javaw.exe process: " << targetProcessId);
    } else {
        std::wcout << L"Multiple javaw.exe processes found:" << std::endl;
        for (size_t i = 0; i < processIds.size(); ++i) {
            std::wcout << L"  " << i + 1 << L". PID: " << processIds[i] << std::endl;
        }
        std::wcout << L"Select process (1-" << processIds.size() << L"): ";
        
        int selection;
        std::wcin >> selection;
        
        if (selection < 1 || selection > static_cast<int>(processIds.size())) {
            LOG_ERROR(L"Invalid selection");
            return 1;
        }
        
        targetProcessId = processIds[selection - 1];
    }

    // 执行注入
    LOG_INFO(L"Injecting JAR: " << jarPath);
    LOG_INFO(L"Target Class: " << className);
    LOG_INFO(L"Target Method: " << methodName);
    LOG_INFO(L"Hot Reload: " << (enableHotReload ? L"Enabled" : L"Disabled"));

    if (injector.InjectDll(targetProcessId, dllPath, injectionData)) {
        LOG_INFO(L"Injection completed successfully!");
        
        if (enableHotReload) {
            std::wcout << L"Hot reload is enabled. Press any key to stop monitoring..." << std::endl;
            std::wcin.get();
            std::wcin.get(); // 消费换行符
        }
    } else {
        LOG_ERROR(L"Injection failed!");
        return 1;
    }

    return 0;
}