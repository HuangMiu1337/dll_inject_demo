#include <gtest/gtest.h>
#include <windows.h>
#include "../../src/include/common.h"

// 初始化测试环境
class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // 设置日志级别为DEBUG以便测试时查看详细信息
        Logger::GetInstance().SetLogLevel(LogLevel::DEBUG);
        
        // 设置测试日志文件
        Logger::GetInstance().SetLogFile(L"test_log.txt");
        
        LOG_INFO(L"Test environment initialized");
    }
    
    void TearDown() override {
        LOG_INFO(L"Test environment cleanup");
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // 添加全局测试环境
    ::testing::AddGlobalTestEnvironment(new TestEnvironment);
    
    return RUN_ALL_TESTS();
}