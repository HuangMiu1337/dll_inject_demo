#pragma once

#include "common.h"
#include <jni.h>

class JarLoader {
public:
    JarLoader();
    ~JarLoader();

    // 初始化JVM
    bool InitializeJVM();
    
    // 加载JAR文件
    bool LoadJar(const std::wstring& jarPath);
    
    // 卸载JAR文件
    bool UnloadJar();
    
    // 调用Java方法
    bool CallJavaMethod(const std::string& className, const std::string& methodName, const std::vector<std::string>& args = {});
    
    // 获取JVM实例
    JavaVM* GetJVM() const { return jvm_; }
    
    // 获取JNI环境
    JNIEnv* GetJNIEnv() const { return env_; }
    
    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

private:
    JavaVM* jvm_;
    JNIEnv* env_;
    bool initialized_;
    std::wstring currentJarPath_;
    
    // 创建JVM参数
    void SetupJVMArgs(JavaVMInitArgs& vmArgs, std::vector<JavaVMOption>& options, const std::wstring& jarPath);
    
    // 查找Java类
    jclass FindClass(const std::string& className);
    
    // 查找Java方法
    jmethodID FindMethod(jclass clazz, const std::string& methodName, const std::string& signature);
};