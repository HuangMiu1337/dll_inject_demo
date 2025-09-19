/**
 * Native Bridge class for JNI communication
 * This class provides methods to interact with the native C++ code
 */
public class NativeBridge {
    
    // 静态块用于加载本地库
    static {
        try {
            // 注意：在DLL注入的情况下，本地方法会通过JNI注册
            System.out.println("NativeBridge class loaded");
        } catch (Exception e) {
            System.err.println("Failed to initialize NativeBridge: " + e.getMessage());
        }
    }
    
    /**
     * 向本地代码发送日志消息
     * @param message 要记录的消息
     */
    public static native void logMessage(String message);
    
    /**
     * 获取系统信息
     * @return 系统信息字符串
     */
    public static native String getSystemInfo();
    
    /**
     * 执行系统命令（出于安全考虑，实际实现中被禁用）
     * @param command 要执行的命令
     * @return 命令执行结果
     */
    public static native String executeCommand(String command);
    
    /**
     * 检查文件是否存在
     * @param filePath 文件路径
     * @return 文件是否存在
     */
    public static native boolean fileExists(String filePath);
    
    /**
     * 获取当前进程ID
     * @return 进程ID
     */
    public static native int getCurrentProcessId();
    
    /**
     * 获取当前线程ID
     * @return 线程ID
     */
    public static native int getCurrentThreadId();
    
    /**
     * 测试本地方法调用
     */
    public static void testNativeMethods() {
        System.out.println("=== Testing Native Methods ===");
        
        try {
            // 测试日志记录
            logMessage("Test message from Java");
            
            // 测试系统信息获取
            String sysInfo = getSystemInfo();
            System.out.println("System Information:");
            System.out.println(sysInfo);
            
            // 测试文件检查
            boolean exists = fileExists("test.jar");
            System.out.println("test.jar exists: " + exists);
            
            // 测试进程和线程ID获取
            int pid = getCurrentProcessId();
            int tid = getCurrentThreadId();
            System.out.println("Process ID: " + pid);
            System.out.println("Thread ID: " + tid);
            
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Native method call failed: " + e.getMessage());
        }
        
        System.out.println("=== Native Methods Test Completed ===");
    }
}