@echo off
echo Building Java test files...

REM 创建输出目录
if not exist "test\build" mkdir "test\build"

REM 编译Java文件
echo Compiling Java files...
javac -d test\build test\java\*.java

if %ERRORLEVEL% NEQ 0 (
    echo Java compilation failed!
    pause
    exit /b 1
)

REM 创建JAR文件
echo Creating JAR files...
cd test\build
jar cf ..\test.jar *.class
cd ..\..

REM 创建目标应用程序JAR
cd test\build
jar cfe ..\target-app.jar TargetApp *.class
cd ..\..

echo Java build completed successfully!
echo Files created:
echo   test\test.jar - Test JAR for injection
echo   test\target-app.jar - Target application JAR

pause