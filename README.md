# DLL注入JAR热重载演示项目

这个项目演示了如何使用DLL注入技术将JAR文件注入到运行中的javaw.exe进程，并实现热重载功能。

## 项目特性

- **DLL注入**: 将自定义DLL注入到目标Java进程
- **JAR加载**: 在目标进程中动态加载和执行JAR文件
- **热重载**: 监控JAR文件变化并自动重新加载
- **JNI桥接**: 提供Java和C++之间的双向通信
- **进程管理**: 自动发现和选择目标Java进程

## 项目结构

```
dll_inject_demo/
├── src/
│   ├── include/           # 头文件
│   │   ├── common.h
│   │   ├── dll_injector.h
│   │   ├── jar_loader.h
│   │   └── hot_reload.h
│   ├── injector/          # 注入器源码
│   │   ├── main.cpp
│   │   ├── dll_injector.cpp
│   │   └── process_utils.cpp
│   ├── dll/               # 注入DLL源码
│   │   ├── dllmain.cpp
│   │   ├── jar_loader.cpp
│   │   ├── jni_bridge.cpp
│   │   └── hot_reload.cpp
│   └── common/            # 通用工具
│       └── utils.cpp
├── test/
│   ├── java/              # 测试Java文件
│   │   ├── Main.java
│   │   ├── NativeBridge.java
│   │   └── TargetApp.java
│   ├── test.jar           # 测试JAR文件
│   └── target-app.jar     # 目标应用程序
├── build/                 # 构建输出目录
├── CMakeLists.txt         # CMake配置文件
├── build.bat              # 主构建脚本
├── build_java.bat         # Java构建脚本
└── README.md              # 本文件
```

## 系统要求

- Windows 10/11 (x64)
- Visual Studio 2019/2022 或 Build Tools
- CMake 3.16+
- Java Development Kit (JDK) 8+
- 管理员权限（用于进程注入）

## 构建步骤

1. **克隆或下载项目**
   ```bash
   git clone <repository-url>
   cd dll_inject_demo
   ```

2. **运行构建脚本**
   ```bash
   build.bat
   ```

   构建脚本会自动：
   - 检查依赖项
   - 配置CMake
   - 编译C++代码
   - 编译Java代码
   - 创建JAR文件

## 使用方法

### 基本使用

1. **启动目标应用程序**
   ```bash
   java -jar test\target-app.jar
   ```

2. **运行注入器**
   ```bash
   build\bin\Release\injector.exe test\test.jar
   ```

### 高级使用

```bash
# 指定类和方法
injector.exe test\test.jar Main main true

# 参数说明:
# - test\test.jar: 要注入的JAR文件路径
# - Main: Java类名 (默认: Main)
# - main: Java方法名 (默认: main)
# - true: 启用热重载 (默认: true)
```

### 热重载测试

1. 启动目标应用程序和注入器
2. 修改 `test\java\Main.java` 文件
3. 重新编译: `build_java.bat`
4. 观察自动重新加载

## 核心组件

### 1. DLL注入器 (injector.exe)

负责将DLL注入到目标Java进程：
- 查找javaw.exe进程
- 分配内存并写入DLL路径
- 创建远程线程加载DLL

### 2. 注入DLL (inject.dll)

在目标进程中执行的核心组件：
- 初始化JVM连接
- 加载指定的JAR文件
- 调用Java方法
- 管理热重载

### 3. JAR加载器

处理Java相关操作：
- 连接到现有JVM实例
- 动态加载JAR文件
- 调用Java类和方法
- 异常处理

### 4. 热重载管理器

监控文件变化：
- 监控JAR文件修改时间
- 自动重新加载变更的JAR
- 重新调用指定方法

### 5. JNI桥接

提供Java和C++通信：
- 本地方法注册
- 系统信息获取
- 文件操作
- 进程信息

## 安全注意事项

⚠️ **重要**: 此项目仅用于教育和研究目的

- DLL注入需要管理员权限
- 可能被杀毒软件误报
- 不要用于恶意目的
- 在生产环境中谨慎使用

## 故障排除

### 常见问题

1. **"Failed to open target process"**
   - 确保以管理员身份运行
   - 检查目标进程是否存在

2. **"Failed to initialize JVM"**
   - 确保目标进程是Java应用程序
   - 检查JNI库是否正确链接

3. **"JAR file not found"**
   - 检查JAR文件路径是否正确
   - 确保文件存在且可读

4. **热重载不工作**
   - 检查文件权限
   - 确保JAR文件没有被锁定

### 调试技巧

1. **启用详细日志**
   - 修改 `common.h` 中的日志级别
   - 重新编译项目

2. **使用调试器**
   - 在Visual Studio中附加到目标进程
   - 设置断点调试

3. **检查进程**
   - 使用Process Explorer查看DLL加载情况
   - 检查Java进程的类路径

## 扩展功能

### 添加新的本地方法

1. 在 `NativeBridge.java` 中声明方法
2. 在 `jni_bridge.cpp` 中实现方法
3. 重新编译项目

### 自定义JAR加载逻辑

1. 修改 `jar_loader.cpp` 中的加载逻辑
2. 添加新的类查找策略
3. 实现自定义类加载器

### 增强热重载功能

1. 添加目录监控
2. 支持多文件监控
3. 实现增量重载

## 许可证

本项目仅供学习和研究使用。请遵守相关法律法规。

## 贡献

欢迎提交问题报告和改进建议。

---

**免责声明**: 本项目仅用于教育目的。使用者需要对使用本项目造成的任何后果负责。