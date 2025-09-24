#pragma once

#include "common.h"
#include "security_utils.h"
#include <jni.h>
#include <unordered_map>

// JNI异常安全包装器
class JNILocalRef {
public:
    JNILocalRef(JNIEnv* env, jobject obj) : env_(env), obj_(obj) {}
    
    ~JNILocalRef() {
        if (env_ && obj_) {
            env_->DeleteLocalRef(obj_);
        }
    }
    
    JNILocalRef(const JNILocalRef&) = delete;
    JNILocalRef& operator=(const JNILocalRef&) = delete;
    
    JNILocalRef(JNILocalRef&& other) noexcept : env_(other.env_), obj_(other.obj_) {
        other.env_ = nullptr;
        other.obj_ = nullptr;
    }
    
    jobject get() const { return obj_; }
    operator bool() const { return obj_ != nullptr; }
    
private:
    JNIEnv* env_;
    jobject obj_;
};

class JarLoader {
public:
    JarLoader();
    ~JarLoader();

    // 禁用拷贝构造和赋值
    JarLoader(const JarLoader&) = delete;
    JarLoader& operator=(const JarLoader&) = delete;

    // 初始化JVM
    ErrorCode InitializeJVM();
    
    // 加载JAR文件
    ErrorCode LoadJar(const std::wstring& jarPath);
    
    // 卸载JAR文件
    ErrorCode UnloadJar();
    
    // 调用Java方法
    ErrorCode CallJavaMethod(const std::string& className, const std::string& methodName, const std::vector<std::string>& args = {});
    
    // 获取JVM实例
    JavaVM* GetJVM() const { return jvm_; }
    
    // 获取JNI环境
    JNIEnv* GetJNIEnv() const { return env_; }
    
    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }
    
    // 获取当前加载的JAR路径
    const std::wstring& GetCurrentJarPath() const { return currentJarPath_; }
    
    // 获取最后的错误码
    ErrorCode GetLastError() const { return lastError_; }

private:
    JavaVM* jvm_;
    JNIEnv* env_;
    bool initialized_;
    std::wstring currentJarPath_;
    ErrorCode lastError_;
    jobject classLoader_;  // URLClassLoader实例
    std::unordered_map<std::string, jclass> classCache_;  // 类缓存
    std::mutex jniMutex_;  // JNI操作互斥锁
    
    // 设置错误码
    void SetLastError(ErrorCode error) { lastError_ = error; }
    
    // 创建JVM参数
    ErrorCode SetupJVMArgs(JavaVMInitArgs& vmArgs, std::vector<JavaVMOption>& options, const std::wstring& jarPath);
    
    // 查找Java类（带缓存）
    jclass FindClass(const std::string& className);
    
    // 查找Java方法
    jmethodID FindMethod(jclass clazz, const std::string& methodName, const std::string& signature);
    
    // 检查并清理JNI异常
    bool CheckAndClearJNIException();
    
    // 创建URLClassLoader
    ErrorCode CreateURLClassLoader(const std::wstring& jarPath);
    
    // 清理资源
    void Cleanup();
    
    // 验证输入参数
    bool ValidateClassName(const std::string& className);
    bool ValidateMethodName(const std::string& methodName);
};