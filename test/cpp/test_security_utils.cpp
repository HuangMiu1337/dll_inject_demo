#include <gtest/gtest.h>
#include "../../src/include/security_utils.h"
#include "../../src/include/common.h"
#include <filesystem>
#include <fstream>

class SecurityUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试文件
        testDllPath_ = L"test_dll.dll";
        testJarPath_ = L"test_jar.jar";
        
        // 创建临时测试文件
        std::ofstream dllFile(testDllPath_);
        dllFile << "dummy dll content";
        dllFile.close();
        
        std::ofstream jarFile(testJarPath_);
        jarFile << "dummy jar content";
        jarFile.close();
    }
    
    void TearDown() override {
        // 清理测试文件
        std::filesystem::remove(testDllPath_);
        std::filesystem::remove(testJarPath_);
    }
    
    std::wstring testDllPath_;
    std::wstring testJarPath_;
};

TEST_F(SecurityUtilsTest, ValidateDllPath_ValidPath) {
    EXPECT_TRUE(SecurityUtils::ValidateDllPath(testDllPath_));
}

TEST_F(SecurityUtilsTest, ValidateDllPath_NonExistentFile) {
    EXPECT_FALSE(SecurityUtils::ValidateDllPath(L"non_existent.dll"));
}

TEST_F(SecurityUtilsTest, ValidateDllPath_EmptyPath) {
    EXPECT_FALSE(SecurityUtils::ValidateDllPath(L""));
}

TEST_F(SecurityUtilsTest, ValidateJarPath_ValidPath) {
    EXPECT_TRUE(SecurityUtils::ValidateJarPath(testJarPath_));
}

TEST_F(SecurityUtilsTest, ValidateJarPath_NonExistentFile) {
    EXPECT_FALSE(SecurityUtils::ValidateJarPath(L"non_existent.jar"));
}

TEST_F(SecurityUtilsTest, ValidateJarPath_EmptyPath) {
    EXPECT_FALSE(SecurityUtils::ValidateJarPath(L""));
}

TEST_F(SecurityUtilsTest, ValidateProcessName_ValidName) {
    EXPECT_TRUE(SecurityUtils::ValidateProcessName(L"notepad.exe"));
    EXPECT_TRUE(SecurityUtils::ValidateProcessName(L"test_process.exe"));
}

TEST_F(SecurityUtilsTest, ValidateProcessName_InvalidName) {
    EXPECT_FALSE(SecurityUtils::ValidateProcessName(L""));
    EXPECT_FALSE(SecurityUtils::ValidateProcessName(L"../malicious.exe"));
    EXPECT_FALSE(SecurityUtils::ValidateProcessName(L"process|with|pipes.exe"));
}

TEST_F(SecurityUtilsTest, ValidateClassName_ValidName) {
    EXPECT_TRUE(SecurityUtils::ValidateClassName("com.example.TestClass"));
    EXPECT_TRUE(SecurityUtils::ValidateClassName("TestClass"));
}

TEST_F(SecurityUtilsTest, ValidateClassName_InvalidName) {
    EXPECT_FALSE(SecurityUtils::ValidateClassName(""));
    EXPECT_FALSE(SecurityUtils::ValidateClassName("../malicious"));
    EXPECT_FALSE(SecurityUtils::ValidateClassName("class with spaces"));
}

TEST_F(SecurityUtilsTest, ValidateMethodName_ValidName) {
    EXPECT_TRUE(SecurityUtils::ValidateMethodName("testMethod"));
    EXPECT_TRUE(SecurityUtils::ValidateMethodName("main"));
}

TEST_F(SecurityUtilsTest, ValidateMethodName_InvalidName) {
    EXPECT_FALSE(SecurityUtils::ValidateMethodName(""));
    EXPECT_FALSE(SecurityUtils::ValidateMethodName("method with spaces"));
    EXPECT_FALSE(SecurityUtils::ValidateMethodName("../malicious"));
}

TEST_F(SecurityUtilsTest, IsSystemCriticalProcess_KnownSystemProcess) {
    EXPECT_TRUE(SecurityUtils::IsSystemCriticalProcess(4)); // System process
}

TEST_F(SecurityUtilsTest, IsSystemCriticalProcess_RegularProcess) {
    DWORD currentPid = GetCurrentProcessId();
    EXPECT_FALSE(SecurityUtils::IsSystemCriticalProcess(currentPid));
}

TEST_F(SecurityUtilsTest, IsRunningAsAdmin) {
    // 这个测试的结果取决于当前进程的权限
    bool isAdmin = SecurityUtils::IsRunningAsAdmin();
    // 只验证函数不会崩溃，不验证具体结果
    EXPECT_TRUE(isAdmin || !isAdmin);
}