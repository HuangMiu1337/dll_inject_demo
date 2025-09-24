#include <gtest/gtest.h>
#include "../../src/include/jar_loader.h"
#include "../../src/include/common.h"
#include <filesystem>
#include <fstream>

class JarLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        jarLoader_ = std::make_unique<JarLoader>();
        
        // 创建测试JAR文件
        testJarPath_ = L"test.jar";
        std::ofstream jarFile(testJarPath_);
        jarFile << "dummy jar content";
        jarFile.close();
    }
    
    void TearDown() override {
        jarLoader_.reset();
        std::filesystem::remove(testJarPath_);
    }
    
    std::unique_ptr<JarLoader> jarLoader_;
    std::wstring testJarPath_;
};

TEST_F(JarLoaderTest, Constructor_InitialState) {
    EXPECT_EQ(jarLoader_->GetLastError(), ErrorCode::SUCCESS);
}

TEST_F(JarLoaderTest, InitializeJVM_Success) {
    ErrorCode result = jarLoader_->InitializeJVM();
    // JVM初始化可能失败（如果没有Java运行时），但不应该崩溃
    EXPECT_TRUE(result == ErrorCode::SUCCESS || result == ErrorCode::JVM_INIT_FAILED);
}

TEST_F(JarLoaderTest, LoadJar_WithoutJVMInitialization) {
    ErrorCode result = jarLoader_->LoadJar(testJarPath_);
    EXPECT_EQ(result, ErrorCode::JVM_NOT_INITIALIZED);
}

TEST_F(JarLoaderTest, LoadJar_EmptyPath) {
    jarLoader_->InitializeJVM();
    ErrorCode result = jarLoader_->LoadJar(L"");
    EXPECT_EQ(result, ErrorCode::SECURITY_CHECK_FAILED);
}

TEST_F(JarLoaderTest, LoadJar_NonExistentFile) {
    jarLoader_->InitializeJVM();
    ErrorCode result = jarLoader_->LoadJar(L"non_existent.jar");
    EXPECT_EQ(result, ErrorCode::SECURITY_CHECK_FAILED);
}

TEST_F(JarLoaderTest, CallJavaMethod_WithoutJVMInitialization) {
    ErrorCode result = jarLoader_->CallJavaMethod("TestClass", "testMethod", {});
    EXPECT_EQ(result, ErrorCode::JVM_NOT_INITIALIZED);
}

TEST_F(JarLoaderTest, CallJavaMethod_InvalidClassName) {
    jarLoader_->InitializeJVM();
    ErrorCode result = jarLoader_->CallJavaMethod("", "testMethod", {});
    EXPECT_EQ(result, ErrorCode::SECURITY_CHECK_FAILED);
}

TEST_F(JarLoaderTest, CallJavaMethod_InvalidMethodName) {
    jarLoader_->InitializeJVM();
    ErrorCode result = jarLoader_->CallJavaMethod("TestClass", "", {});
    EXPECT_EQ(result, ErrorCode::SECURITY_CHECK_FAILED);
}

TEST_F(JarLoaderTest, ValidateInput_ValidInput) {
    // 使用反射访问私有方法进行测试（这里简化为公共接口测试）
    ErrorCode result = jarLoader_->CallJavaMethod("ValidClass", "validMethod", {"validArg"});
    // 应该返回JVM_NOT_INITIALIZED而不是SECURITY_CHECK_FAILED
    EXPECT_EQ(result, ErrorCode::JVM_NOT_INITIALIZED);
}

TEST_F(JarLoaderTest, ErrorHandling_GetLastError) {
    // 触发一个错误
    jarLoader_->LoadJar(L"");
    EXPECT_EQ(jarLoader_->GetLastError(), ErrorCode::SECURITY_CHECK_FAILED);
}

// 性能测试
TEST_F(JarLoaderTest, Performance_MultipleMethodCalls) {
    jarLoader_->InitializeJVM();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 多次调用相同方法，测试缓存效果
    for (int i = 0; i < 100; ++i) {
        jarLoader_->CallJavaMethod("TestClass", "testMethod", {});
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 验证性能在合理范围内（这里只是示例，实际阈值需要根据环境调整）
    EXPECT_LT(duration.count(), 5000); // 应该在5秒内完成
}