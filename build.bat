@echo off
echo ========================================
echo DLL Injection Demo - Build Script
echo ========================================

REM 检查CMake是否可用
cmake --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found! Please install CMake and add it to PATH.
    pause
    exit /b 1
)

REM 检查Visual Studio构建工具
where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Setting up Visual Studio environment...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    if %ERRORLEVEL% NEQ 0 (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
        if %ERRORLEVEL% NEQ 0 (
            echo ERROR: Visual Studio not found! Please install Visual Studio or Build Tools.
            pause
            exit /b 1
        )
    )
)

REM 创建构建目录
if not exist "build" mkdir "build"
cd build

REM 运行CMake配置
echo Configuring project with CMake...
cmake .. -G "Visual Studio 16 2019" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed! Trying with Visual Studio 2022...
    cmake .. -G "Visual Studio 17 2022" -A x64
    if %ERRORLEVEL% NEQ 0 (
        echo CMake configuration failed!
        cd ..
        pause
        exit /b 1
    )
)

REM 构建项目
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

REM 构建Java文件
echo Building Java test files...
call build_java.bat

echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Output files:
echo   build\bin\Release\injector.exe - DLL Injector
echo   build\lib\Release\inject.dll   - Injection DLL
echo   test\test.jar                   - Test JAR for injection
echo   test\target-app.jar             - Target application
echo.
echo Usage:
echo   1. Run target application: java -jar test\target-app.jar
echo   2. Run injector: build\bin\Release\injector.exe test\test.jar
echo.
pause