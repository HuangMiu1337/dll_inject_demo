public class Main {
    private static int counter = 0;
    
    public static void main(String[] args) {
        System.out.println("=== DLL Injection Demo - JAR Loaded Successfully ===");
        System.out.println("Counter: " + (++counter));
        System.out.println("Current Time: " + new java.util.Date());
        System.out.println("Process ID: " + ProcessHandle.current().pid());
        
        // 测试参数
        if (args != null && args.length > 0) {
            System.out.println("Arguments received:");
            for (int i = 0; i < args.length; i++) {
                System.out.println("  [" + i + "] " + args[i]);
            }
        }
        
        // 测试本地方法调用（如果NativeBridge类可用）
        try {
            testNativeBridge();
        } catch (Exception e) {
            System.out.println("Native bridge not available: " + e.getMessage());
        }
        
        // 启动一个简单的后台任务来演示热重载
        startBackgroundTask();
        
        System.out.println("=== JAR Execution Completed ===");
    }
    
    private static void testNativeBridge() {
        try {
            // 尝试调用本地方法
            NativeBridge.logMessage("Hello from Java!");
            String systemInfo = NativeBridge.getSystemInfo();
            System.out.println("System Info from Native:\n" + systemInfo);
            
            int processId = NativeBridge.getCurrentProcessId();
            int threadId = NativeBridge.getCurrentThreadId();
            System.out.println("Native Process ID: " + processId);
            System.out.println("Native Thread ID: " + threadId);
            
        } catch (UnsatisfiedLinkError e) {
            System.out.println("Native methods not loaded: " + e.getMessage());
        }
    }
    
    private static void startBackgroundTask() {
        Thread backgroundThread = new Thread(() -> {
            try {
                for (int i = 0; i < 5; i++) {
                    Thread.sleep(2000);
                    System.out.println("Background task iteration: " + (i + 1) + " (Counter: " + counter + ")");
                }
            } catch (InterruptedException e) {
                System.out.println("Background task interrupted");
            }
        });
        
        backgroundThread.setDaemon(true);
        backgroundThread.start();
    }
    
    // 无参数方法，用于测试热重载
    public static void reload() {
        System.out.println("=== Hot Reload Method Called ===");
        System.out.println("Reload Counter: " + (++counter));
        System.out.println("Reload Time: " + new java.util.Date());
        System.out.println("=== Hot Reload Completed ===");
    }
}