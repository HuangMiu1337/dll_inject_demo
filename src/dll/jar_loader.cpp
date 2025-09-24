#include "../include/jar_loader.h"
#include "../include/common.h"
#include <shlwapi.h>
#include <mutex>
#include <unordered_map>

JarLoader::JarLoader() 
    : jvm_(nullptr), env_(nullptr), initialized_(false), 
      lastError_(ErrorCode::SUCCESS), classLoader_(nullptr) {
    LOG_DEBUG(L"JarLoader created");
}

JarLoader::~JarLoader() {
    LOG_DEBUG(L"JarLoader destroying...");
    Cleanup();
    LOG_DEBUG(L"JarLoader destroyed");
}

ErrorCode JarLoader::InitializeJVM() {
    std::lock_guard<std::mutex> lock(jniMutex_);
    
    if (initialized_) {
        LOG_INFO(L"JVM already initialized");
        SetLastError(ErrorCode::SUCCESS);
        return ErrorCode::SUCCESS;
    }

    try {
        // 检查是否已经有JVM实例
        JavaVM* existingJVMs[1];
        jsize numJVMs = 0;
        jint result = JNI_GetCreatedJavaVMs(existingJVMs, 1, &numJVMs);
        
        if (result == JNI_OK && numJVMs > 0) {
            // 使用现有的JVM
            jvm_ = existingJVMs[0];
            if (!jvm_) {
                LOG_ERROR(L"Retrieved JVM pointer is null");
                SetLastError(ErrorCode::JVM_INIT_FAILED);
                return ErrorCode::JVM_INIT_FAILED;
            }
            
            result = jvm_->AttachCurrentThread((void**)&env_, NULL);
            if (result != JNI_OK) {
                LOG_ERROR(L"Failed to attach to existing JVM thread, error code: " << result);
                SetLastError(ErrorCode::JVM_INIT_FAILED);
                return ErrorCode::JVM_INIT_FAILED;
            }
            
            if (!env_) {
                LOG_ERROR(L"JNI environment is null after attach");
                SetLastError(ErrorCode::JVM_INIT_FAILED);
                return ErrorCode::JVM_INIT_FAILED;
            }
            
            LOG_INFO(L"Attached to existing JVM successfully");
        } else {
            // 创建新的JVM（通常不会发生，因为我们注入到已有的Java进程）
            LOG_INFO(L"No existing JVM found, creating new one");
            
            JavaVMInitArgs vmArgs;
            std::vector<JavaVMOption> options;
            ErrorCode setupResult = SetupJVMArgs(vmArgs, options, L"");
            if (setupResult != ErrorCode::SUCCESS) {
                LOG_ERROR(L"Failed to setup JVM arguments");
                SetLastError(setupResult);
                return setupResult;
            }
            
            result = JNI_CreateJavaVM(&jvm_, (void**)&env_, &vmArgs);
            if (result != JNI_OK) {
                LOG_ERROR(L"Failed to create JVM, error code: " << result);
                SetLastError(ErrorCode::JVM_INIT_FAILED);
                return ErrorCode::JVM_INIT_FAILED;
            }
            
            if (!jvm_ || !env_) {
                LOG_ERROR(L"JVM or JNI environment is null after creation");
                SetLastError(ErrorCode::JVM_INIT_FAILED);
                return ErrorCode::JVM_INIT_FAILED;
            }
        }
        
        // 检查JNI版本
        jint version = env_->GetVersion();
        LOG_INFO(L"JNI version: " << std::hex << version);
        
        initialized_ = true;
        SetLastError(ErrorCode::SUCCESS);
        LOG_INFO(L"JVM initialization successful");
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during JVM initialization: " << StringToWString(e.what()));
        SetLastError(ErrorCode::JVM_INIT_FAILED);
        return ErrorCode::JVM_INIT_FAILED;
    } catch (...) {
        LOG_ERROR(L"Unknown exception during JVM initialization");
        SetLastError(ErrorCode::UNKNOWN_ERROR);
        return ErrorCode::UNKNOWN_ERROR;
    }
}

bool JarLoader::LoadJar(const std::wstring& jarPath) {
    if (!initialized_) {
        LOG_ERROR(L"JVM not initialized");
        return false;
    }
    
    if (!FileExists(jarPath)) {
        LOG_ERROR(L"JAR file not found: " << jarPath);
        return false;
    }
    
    try {
        // 将JAR路径转换为URL格式
        std::string jarPathStr = WStringToString(jarPath);
        std::string jarUrl = "file:///" + jarPathStr;
        
        // 替换反斜杠为正斜杠
        for (char& c : jarUrl) {
            if (c == '\\') c = '/';
        }
        
        // 获取URLClassLoader类
        jclass urlClassLoaderClass = env_->FindClass("java/net/URLClassLoader");
        if (!urlClassLoaderClass) {
            LOG_ERROR(L"Failed to find URLClassLoader class");
            return false;
        }
        
        // 获取URL类
        jclass urlClass = env_->FindClass("java/net/URL");
        if (!urlClass) {
            LOG_ERROR(L"Failed to find URL class");
            return false;
        }
        
        // 创建URL对象
        jmethodID urlConstructor = env_->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
        jstring jarUrlString = env_->NewStringUTF(jarUrl.c_str());
        jobject urlObject = env_->NewObject(urlClass, urlConstructor, jarUrlString);
        
        if (!urlObject) {
            LOG_ERROR(L"Failed to create URL object");
            return false;
        }
        
        // 创建URL数组
        jobjectArray urlArray = env_->NewObjectArray(1, urlClass, urlObject);
        
        // 获取URLClassLoader构造函数
        jmethodID classLoaderConstructor = env_->GetMethodID(urlClassLoaderClass, "<init>", "([Ljava/net/URL;)V");
        jobject classLoader = env_->NewObject(urlClassLoaderClass, classLoaderConstructor, urlArray);
        
        if (!classLoader) {
            LOG_ERROR(L"Failed to create URLClassLoader");
            return false;
        }
        
        // 设置当前线程的类加载器
        jclass threadClass = env_->FindClass("java/lang/Thread");
        jmethodID currentThreadMethod = env_->GetStaticMethodID(threadClass, "currentThread", "()Ljava/lang/Thread;");
        jobject currentThread = env_->CallStaticObjectMethod(threadClass, currentThreadMethod);
        
        jmethodID setContextClassLoaderMethod = env_->GetMethodID(threadClass, "setContextClassLoader", "(Ljava/lang/ClassLoader;)V");
        env_->CallVoidMethod(currentThread, setContextClassLoaderMethod, classLoader);
        
        currentJarPath_ = jarPath;
        LOG_INFO(L"JAR loaded successfully: " << jarPath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during JAR loading: " << StringToWString(e.what()));
        return false;
    }
}

bool JarLoader::UnloadJar() {
    if (!initialized_) {
        return true;
    }
    
    try {
        // 注意：Java中卸载类加载器是复杂的，这里只是简单的清理
        currentJarPath_.clear();
        LOG_INFO(L"JAR unloaded");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during JAR unloading: " << StringToWString(e.what()));
        return false;
    }
}

bool JarLoader::CallJavaMethod(const std::string& className, const std::string& methodName, const std::vector<std::string>& args) {
    if (!initialized_) {
        LOG_ERROR(L"JVM not initialized");
        return false;
    }
    
    try {
        // 查找类
        jclass clazz = FindClass(className);
        if (!clazz) {
            LOG_ERROR(L"Failed to find class: " << StringToWString(className));
            return false;
        }
        
        // 构建方法签名
        std::string signature = "([Ljava/lang/String;)V"; // 假设是main方法签名
        if (methodName != "main") {
            signature = "()V"; // 无参数方法
        }
        
        // 查找方法
        jmethodID method = FindMethod(clazz, methodName, signature);
        if (!method) {
            LOG_ERROR(L"Failed to find method: " << StringToWString(methodName));
            return false;
        }
        
        // 准备参数
        jobjectArray argsArray = nullptr;
        if (methodName == "main") {
            jclass stringClass = env_->FindClass("java/lang/String");
            argsArray = env_->NewObjectArray(args.size(), stringClass, nullptr);
            
            for (size_t i = 0; i < args.size(); ++i) {
                jstring argString = env_->NewStringUTF(args[i].c_str());
                env_->SetObjectArrayElement(argsArray, i, argString);
            }
        }
        
        // 调用方法
        if (methodName == "main") {
            env_->CallStaticVoidMethod(clazz, method, argsArray);
        } else {
            // 创建类实例并调用实例方法
            jmethodID constructor = env_->GetMethodID(clazz, "<init>", "()V");
            if (constructor) {
                jobject instance = env_->NewObject(clazz, constructor);
                if (instance) {
                    env_->CallVoidMethod(instance, method);
                }
            } else {
                // 尝试作为静态方法调用
                env_->CallStaticVoidMethod(clazz, method);
            }
        }
        
        // 检查异常
        if (env_->ExceptionCheck()) {
            env_->ExceptionDescribe();
            env_->ExceptionClear();
            LOG_ERROR(L"Java exception occurred during method call");
            return false;
        }
        
        LOG_INFO(L"Java method called successfully: " << StringToWString(className + "." + methodName));
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during Java method call: " << StringToWString(e.what()));
        return false;
    }
}

void JarLoader::SetupJVMArgs(JavaVMInitArgs& vmArgs, std::vector<JavaVMOption>& options, const std::wstring& jarPath) {
    vmArgs.version = JNI_VERSION_1_8;
    vmArgs.nOptions = 0;
    vmArgs.options = nullptr;
    vmArgs.ignoreUnrecognized = JNI_FALSE;
    
    if (!jarPath.empty()) {
        std::string classPath = "-Djava.class.path=" + WStringToString(jarPath);
        options.push_back({const_cast<char*>(classPath.c_str()), nullptr});
        vmArgs.nOptions = options.size();
        vmArgs.options = options.data();
    }
}

jclass JarLoader::FindClass(const std::string& className) {
    if (!env_) return nullptr;
    
    // 将点号替换为斜杠
    std::string jniClassName = className;
    for (char& c : jniClassName) {
        if (c == '.') c = '/';
    }
    
    return env_->FindClass(jniClassName.c_str());
}

jmethodID JarLoader::FindMethod(jclass clazz, const std::string& methodName, const std::string& signature) {
    if (!env_ || !clazz) {
        return nullptr;
    }
    
    jmethodID method = env_->GetStaticMethodID(clazz, methodName.c_str(), signature.c_str());
    if (!method) {
        LOG_ERROR(L"Method not found: " << StringToWString(methodName));
    }
    
    return method;
}

// 新增的辅助方法实现
ErrorCode JarLoader::SetupJVMArgs(JavaVMInitArgs& vmArgs, std::vector<JavaVMOption>& options, const std::wstring& jarPath) {
    try {
        options.clear();
        
        // 基本JVM选项
        JavaVMOption option1;
        option1.optionString = const_cast<char*>("-Djava.class.path=.");
        options.push_back(option1);
        
        // 如果提供了JAR路径，添加到classpath
        std::string jarPathStr;
        if (!jarPath.empty()) {
            jarPathStr = WStringToString(jarPath);
            std::string classpathOption = "-Djava.class.path=" + jarPathStr;
            JavaVMOption jarOption;
            jarOption.optionString = const_cast<char*>(classpathOption.c_str());
            options.push_back(jarOption);
        }
        
        vmArgs.version = JNI_VERSION_1_8;
        vmArgs.nOptions = static_cast<jint>(options.size());
        vmArgs.options = options.data();
        vmArgs.ignoreUnrecognized = JNI_FALSE;
        
        return ErrorCode::SUCCESS;
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception in SetupJVMArgs: " << StringToWString(e.what()));
        return ErrorCode::INVALID_PARAMETER;
    }
}

bool JarLoader::CheckJNIException() {
    if (!env_) {
        return false;
    }
    
    if (env_->ExceptionCheck()) {
        jthrowable exception = env_->ExceptionOccurred();
        if (exception) {
            env_->ExceptionDescribe();
            env_->ExceptionClear();
            LOG_ERROR(L"JNI exception detected and cleared");
        }
        return true;
    }
    return false;
}

ErrorCode JarLoader::CreateClassLoader(const std::wstring& jarPath) {
    if (!env_ || jarPath.empty()) {
        return ErrorCode::INVALID_PARAMETER;
    }
    
    try {
        // 创建File对象
        jclass fileClass = env_->FindClass("java/io/File");
        if (!fileClass || CheckJNIException()) {
            LOG_ERROR(L"Failed to find File class");
            return ErrorCode::CLASS_NOT_FOUND;
        }
        
        jmethodID fileConstructor = env_->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
        if (!fileConstructor || CheckJNIException()) {
            LOG_ERROR(L"Failed to find File constructor");
            return ErrorCode::METHOD_NOT_FOUND;
        }
        
        std::string jarPathStr = WStringToString(jarPath);
        jstring jarPathJStr = env_->NewStringUTF(jarPathStr.c_str());
        if (!jarPathJStr || CheckJNIException()) {
            LOG_ERROR(L"Failed to create jar path string");
            return ErrorCode::MEMORY_ALLOCATION_FAILED;
        }
        
        jobject jarFile = env_->NewObject(fileClass, fileConstructor, jarPathJStr);
        if (!jarFile || CheckJNIException()) {
            LOG_ERROR(L"Failed to create File object");
            env_->DeleteLocalRef(jarPathJStr);
            return ErrorCode::OBJECT_CREATION_FAILED;
        }
        
        // 创建URL对象
        jmethodID toURIMethod = env_->GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
        if (!toURIMethod || CheckJNIException()) {
            LOG_ERROR(L"Failed to find toURI method");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            return ErrorCode::METHOD_NOT_FOUND;
        }
        
        jobject uri = env_->CallObjectMethod(jarFile, toURIMethod);
        if (!uri || CheckJNIException()) {
            LOG_ERROR(L"Failed to get URI");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            return ErrorCode::METHOD_CALL_FAILED;
        }
        
        jclass uriClass = env_->FindClass("java/net/URI");
        jmethodID toURLMethod = env_->GetMethodID(uriClass, "toURL", "()Ljava/net/URL;");
        if (!toURLMethod || CheckJNIException()) {
            LOG_ERROR(L"Failed to find toURL method");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            env_->DeleteLocalRef(uri);
            return ErrorCode::METHOD_NOT_FOUND;
        }
        
        jobject url = env_->CallObjectMethod(uri, toURLMethod);
        if (!url || CheckJNIException()) {
            LOG_ERROR(L"Failed to get URL");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            env_->DeleteLocalRef(uri);
            return ErrorCode::METHOD_CALL_FAILED;
        }
        
        // 创建URLClassLoader
        jclass urlClassLoaderClass = env_->FindClass("java/net/URLClassLoader");
        if (!urlClassLoaderClass || CheckJNIException()) {
            LOG_ERROR(L"Failed to find URLClassLoader class");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            env_->DeleteLocalRef(uri);
            env_->DeleteLocalRef(url);
            return ErrorCode::CLASS_NOT_FOUND;
        }
        
        jmethodID urlClassLoaderConstructor = env_->GetMethodID(urlClassLoaderClass, "<init>", "([Ljava/net/URL;)V");
        if (!urlClassLoaderConstructor || CheckJNIException()) {
            LOG_ERROR(L"Failed to find URLClassLoader constructor");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            env_->DeleteLocalRef(uri);
            env_->DeleteLocalRef(url);
            return ErrorCode::METHOD_NOT_FOUND;
        }
        
        // 创建URL数组
        jclass urlClass = env_->FindClass("java/net/URL");
        jobjectArray urlArray = env_->NewObjectArray(1, urlClass, url);
        if (!urlArray || CheckJNIException()) {
            LOG_ERROR(L"Failed to create URL array");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            env_->DeleteLocalRef(uri);
            env_->DeleteLocalRef(url);
            return ErrorCode::MEMORY_ALLOCATION_FAILED;
        }
        
        // 创建ClassLoader
        jobject newClassLoader = env_->NewObject(urlClassLoaderClass, urlClassLoaderConstructor, urlArray);
        if (!newClassLoader || CheckJNIException()) {
            LOG_ERROR(L"Failed to create URLClassLoader");
            env_->DeleteLocalRef(jarPathJStr);
            env_->DeleteLocalRef(jarFile);
            env_->DeleteLocalRef(uri);
            env_->DeleteLocalRef(url);
            env_->DeleteLocalRef(urlArray);
            return ErrorCode::OBJECT_CREATION_FAILED;
        }
        
        // 清理旧的ClassLoader
        if (classLoader_) {
            env_->DeleteGlobalRef(classLoader_);
        }
        
        // 保存新的ClassLoader为全局引用
        classLoader_ = env_->NewGlobalRef(newClassLoader);
        
        // 清理本地引用
        env_->DeleteLocalRef(jarPathJStr);
        env_->DeleteLocalRef(jarFile);
        env_->DeleteLocalRef(uri);
        env_->DeleteLocalRef(url);
        env_->DeleteLocalRef(urlArray);
        env_->DeleteLocalRef(newClassLoader);
        
        LOG_INFO(L"ClassLoader created successfully for: " << jarPath);
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception in CreateClassLoader: " << StringToWString(e.what()));
        return ErrorCode::UNKNOWN_ERROR;
    }
}

void JarLoader::Cleanup() {
    std::lock_guard<std::mutex> lock(jniMutex_);
    
    try {
        // 清理类缓存
        if (env_) {
            for (auto& pair : classCache_) {
                if (pair.second) {
                    env_->DeleteGlobalRef(pair.second);
                }
            }
        }
        classCache_.clear();
        
        // 清理ClassLoader
        if (classLoader_ && env_) {
            env_->DeleteGlobalRef(classLoader_);
            classLoader_ = nullptr;
        }
        
        // 分离线程（如果是附加的）
        if (jvm_ && initialized_) {
            jvm_->DetachCurrentThread();
        }
        
        jvm_ = nullptr;
        env_ = nullptr;
        initialized_ = false;
        
        LOG_INFO(L"JarLoader cleanup completed");
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during cleanup: " << StringToWString(e.what()));
    } catch (...) {
        LOG_ERROR(L"Unknown exception during cleanup");
    }
}

bool JarLoader::ValidateInput(const std::string& input, size_t maxLength) {
    if (input.empty() || input.length() > maxLength) {
        return false;
    }
    
    // 检查是否包含危险字符
    const std::string dangerousChars = ";<>|&$`";
    for (char c : dangerousChars) {
        if (input.find(c) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

void JarLoader::SetLastError(ErrorCode error) {
    lastError_ = error;
}

ErrorCode JarLoader::GetLastError() const {
    return lastError_;
}