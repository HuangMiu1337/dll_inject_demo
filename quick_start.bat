@echo off
echo ========================================
echo DLL Injection Demo - Quick Start
echo ========================================

REM 检查是否已构建
if not exist "build\bin\Release\injector.exe" (
    echo Project not built yet. Building now...
    call build.bat
    if %ERRORLEVEL% NEQ 0 (
        echo Build failed! Please check the error messages above.
        pause
        exit /b 1
    )
)

if not exist "test\test.jar" (
    echo Test JAR not found. Building Java files...
    call build_java.bat
    if %ERRORLEVEL% NEQ 0 (
        echo Java build failed!
        pause
        exit /b 1
    )
)

echo.
echo ========================================
echo Quick Start Options:
echo ========================================
echo 1. Start Target Application (GUI)
echo 2. Start Target Application (Console)
echo 3. Inject JAR into existing javaw.exe process
echo 4. Build and inject (full demo)
echo 5. Exit
echo.

set /p choice="Please select an option (1-5): "

if "%choice%"=="1" (
    echo Starting target application with GUI...
    start "Target App" java -jar test\target-app.jar
    echo Target application started. You can now run option 3 to inject.
    pause
) else if "%choice%"=="2" (
    echo Starting target application in console...
    echo Note: This will keep the console open. Use Ctrl+C to stop.
    java -cp test\test.jar TargetApp
) else if "%choice%"=="3" (
    echo Injecting JAR into existing javaw.exe process...
    build\bin\Release\injector.exe test\test.jar Main main true
    pause
) else if "%choice%"=="4" (
    echo Running full demo...
    echo.
    echo Step 1: Starting target application...
    start "Target App" java -jar test\target-app.jar
    echo Waiting for application to start...
    timeout /t 3 /nobreak >nul
    
    echo.
    echo Step 2: Injecting JAR...
    build\bin\Release\injector.exe test\test.jar Main main true
    
    echo.
    echo Demo completed! Check the target application window for results.
    pause
) else if "%choice%"=="5" (
    echo Goodbye!
    exit /b 0
) else (
    echo Invalid choice. Please run the script again.
    pause
)

echo.
echo ========================================
echo Quick Start completed!
echo ========================================
pause