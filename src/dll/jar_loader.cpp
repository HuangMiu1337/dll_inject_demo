#include "../include/jar_loader.h"
#include "../include/common.h"
#include <shlwapi.h>

JarLoader::JarLoader() : jvm_(nullptr), env_(nullptr), initialized_(false) {
}

JarLoader::~JarLoader() {
    UnloadJar();
}

bool JarLoader::InitializeJVM() {
    if (initialized_) {
        LOG_INFO(L"JVM already initialized");
        return true;
    }

    try {
        // 检查是否已经有JVM实例
        JavaVM* existingJVMs[1];
        jsize numJVMs;
        jint result = JNI_GetCreatedJavaVMs(existingJVMs, 1, &numJVMs);
        
        if (result == JNI_OK && numJVMs > 0) {
            // 使用现有的JVM
            jvm_ = existingJVMs[0];
            result = jvm_->AttachCurrentThread((void**)&env_, NULL);
            if (result != JNI_OK) {
                LOG_ERROR(L"Failed to attach to existing JVM thread");
                return false;
            }
            LOG_INFO(L"Attached to existing JVM");
        } else {
            // 创建新的JVM（通常不会发生，因为我们注入到已有的Java进程）
            LOG_INFO(L"No existing JVM found, creating new one");
            
            JavaVMInitArgs vmArgs;
            std::vector<JavaVMOption> options;
            SetupJVMArgs(vmArgs, options, L"");
            
            result = JNI_CreateJavaVM(&jvm_, (void**)&env_, &vmArgs);
            if (result != JNI_OK) {
                LOG_ERROR(L"Failed to create JVM, error code: " << result);
                return false;
            }
        }
        
        initialized_ = true;
        LOG_INFO(L"JVM initialization successful");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(L"Exception during JVM initialization: " << StringToWString(e.what()));
        return false;
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
    if (!env_ || !clazz) return nullptr;
    
    // 首先尝试静态方法
    jmethodID method = env_->GetStaticMethodID(clazz, methodName.c_str(), signature.c_str());
    if (method) return method;
    
    // 清除异常并尝试实例方法
    env_->ExceptionClear();
    return env_->GetMethodID(clazz, methodName.c_str(), signature.c_str());
}