# 测试配置
cmake_minimum_required(VERSION 3.20)

# 查找Google Test
find_package(GTest REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${GTEST_INCLUDE_DIRS})

# 创建测试可执行文件
add_executable(unit_tests
    test_main.cpp
    test_security_utils.cpp
    test_jar_loader.cpp
    test_error_handling.cpp
    
    # 包含需要测试的源文件
    ${CMAKE_SOURCE_DIR}/src/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/common/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/dll/security_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/dll/jar_loader.cpp
)

# 链接库
target_link_libraries(unit_tests
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    jvm
    shlwapi
    advapi32
    kernel32
    user32
)

# 设置C++标准
set_property(TARGET unit_tests PROPERTY CXX_STANDARD 17)

# 添加测试
enable_testing()
add_test(NAME UnitTests COMMAND unit_tests)

# 设置测试工作目录
set_tests_properties(UnitTests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# 复制测试数据文件（如果有的话）
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data DESTINATION ${CMAKE_BINARY_DIR})