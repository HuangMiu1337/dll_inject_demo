cmake_minimum_required(VERSION 3.16)
project(DllInjectDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Java和JNI
find_package(JNI REQUIRED)

# 包含目录
include_directories(${JNI_INCLUDE_DIRS})
include_directories(src/include)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# DLL注入器可执行文件
add_executable(injector
    src/injector/main.cpp
    src/injector/process_utils.cpp
    src/injector/dll_injector.cpp
    src/common/utils.cpp
    src/common/logger.cpp
    src/common/security_utils.cpp
)

# 注入DLL
add_library(inject_dll SHARED
    src/dll/dllmain.cpp
    src/dll/jar_loader.cpp
    src/dll/hot_reload.cpp
    src/dll/jni_bridge.cpp
    src/common/utils.cpp
    src/common/logger.cpp
    src/common/security_utils.cpp
)

# 链接库
target_link_libraries(injector 
    kernel32 
    psapi 
    advapi32
)

target_link_libraries(inject_dll 
    ${JNI_LIBRARIES}
    kernel32 
    user32
    shlwapi
)

# 设置DLL输出名称
set_target_properties(inject_dll PROPERTIES OUTPUT_NAME "inject")

# 复制测试文件到输出目录
configure_file(${CMAKE_SOURCE_DIR}/test/test.jar ${CMAKE_BINARY_DIR}/bin/test.jar COPYONLY)