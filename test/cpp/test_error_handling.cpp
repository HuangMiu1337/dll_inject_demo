#include <gtest/gtest.h>
#include "../../src/include/common.h"

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }
    
    void TearDown() override {
        // 清理测试环境
    }
};

TEST_F(ErrorHandlingTest, ErrorDetails_DefaultConstructor) {
    ErrorDetails error;
    EXPECT_EQ(error.code, ErrorCode::SUCCESS);
    EXPECT_TRUE(error.message.empty());
    EXPECT_TRUE(error.context.empty());
    EXPECT_EQ(error.systemError, 0);
    EXPECT_TRUE(error.file.empty());
    EXPECT_EQ(error.line, 0);
}

TEST_F(ErrorHandlingTest, ErrorDetails_ParameterizedConstructor) {
    ErrorDetails error(ErrorCode::INVALID_PARAMETER, L"Test message", L"Test context", 123, "test.cpp", 42);
    
    EXPECT_EQ(error.code, ErrorCode::INVALID_PARAMETER);
    EXPECT_EQ(error.message, L"Test message");
    EXPECT_EQ(error.context, L"Test context");
    EXPECT_EQ(error.systemError, 123);
    EXPECT_EQ(error.file, "test.cpp");
    EXPECT_EQ(error.line, 42);
}

TEST_F(ErrorHandlingTest, ErrorDetails_ToString_BasicMessage) {
    ErrorDetails error(ErrorCode::INVALID_PARAMETER, L"Test message");
    std::wstring result = error.ToString();
    
    EXPECT_TRUE(result.find(L"[4001]") != std::wstring::npos); // ErrorCode::INVALID_PARAMETER = 4001
    EXPECT_TRUE(result.find(L"Test message") != std::wstring::npos);
}

TEST_F(ErrorHandlingTest, ErrorDetails_ToString_WithContext) {
    ErrorDetails error(ErrorCode::INVALID_PARAMETER, L"Test message", L"Test context");
    std::wstring result = error.ToString();
    
    EXPECT_TRUE(result.find(L"Test message") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"Context: Test context") != std::wstring::npos);
}

TEST_F(ErrorHandlingTest, ErrorDetails_ToString_WithSystemError) {
    ErrorDetails error(ErrorCode::INVALID_PARAMETER, L"Test message", L"", 123);
    std::wstring result = error.ToString();
    
    EXPECT_TRUE(result.find(L"Test message") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"System Error: 123") != std::wstring::npos);
}

TEST_F(ErrorHandlingTest, ErrorDetails_ToString_WithFileAndLine) {
    ErrorDetails error(ErrorCode::INVALID_PARAMETER, L"Test message", L"", 0, "test.cpp", 42);
    std::wstring result = error.ToString();
    
    EXPECT_TRUE(result.find(L"Test message") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"test.cpp:42") != std::wstring::npos);
}

TEST_F(ErrorHandlingTest, ErrorDetails_ToString_Complete) {
    ErrorDetails error(ErrorCode::INVALID_PARAMETER, L"Test message", L"Test context", 123, "test.cpp", 42);
    std::wstring result = error.ToString();
    
    EXPECT_TRUE(result.find(L"[4001]") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"Test message") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"Context: Test context") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"System Error: 123") != std::wstring::npos);
    EXPECT_TRUE(result.find(L"test.cpp:42") != std::wstring::npos);
}

TEST_F(ErrorHandlingTest, ErrorCode_Values) {
    // 验证错误代码的值
    EXPECT_EQ(static_cast<int>(ErrorCode::SUCCESS), 0);
    EXPECT_EQ(static_cast<int>(ErrorCode::PROCESS_NOT_FOUND), 1001);
    EXPECT_EQ(static_cast<int>(ErrorCode::JVM_INIT_FAILED), 2001);
    EXPECT_EQ(static_cast<int>(ErrorCode::INVALID_PARAMETER), 4001);
    EXPECT_EQ(static_cast<int>(ErrorCode::UNKNOWN_ERROR), 9999);
}

// 测试宏的使用（编译时测试）
TEST_F(ErrorHandlingTest, ErrorMacros_Compilation) {
    // 这些宏应该能够编译通过
    ErrorDetails error1 = CREATE_ERROR_DETAILS(ErrorCode::INVALID_PARAMETER, L"Test", L"Context");
    ErrorDetails error2 = CREATE_ERROR_DETAILS_NO_SYS(ErrorCode::INVALID_PARAMETER, L"Test", L"Context");
    
    EXPECT_EQ(error1.code, ErrorCode::INVALID_PARAMETER);
    EXPECT_EQ(error2.code, ErrorCode::INVALID_PARAMETER);
    EXPECT_EQ(error2.systemError, 0);
}