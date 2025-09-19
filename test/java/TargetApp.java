import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 * 目标Java应用程序
 * 这个程序将作为DLL注入的目标进程
 */
public class TargetApp extends JFrame {
    private JTextArea logArea;
    private JButton clearButton;
    private Timer timer;
    private int counter = 0;
    
    public TargetApp() {
        initializeUI();
        startTimer();
    }
    
    private void initializeUI() {
        setTitle("DLL Injection Target Application");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(600, 400);
        setLocationRelativeTo(null);
        
        // 创建布局
        setLayout(new BorderLayout());
        
        // 创建日志区域
        logArea = new JTextArea();
        logArea.setEditable(false);
        logArea.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        logArea.setBackground(Color.BLACK);
        logArea.setForeground(Color.GREEN);
        
        JScrollPane scrollPane = new JScrollPane(logArea);
        scrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
        add(scrollPane, BorderLayout.CENTER);
        
        // 创建控制面板
        JPanel controlPanel = new JPanel(new FlowLayout());
        
        clearButton = new JButton("Clear Log");
        clearButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                logArea.setText("");
                logMessage("Log cleared");
            }
        });
        
        JButton infoButton = new JButton("Show Info");
        infoButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                showSystemInfo();
            }
        });
        
        controlPanel.add(clearButton);
        controlPanel.add(infoButton);
        add(controlPanel, BorderLayout.SOUTH);
        
        // 初始日志
        logMessage("Target Application Started");
        logMessage("Process ID: " + ProcessHandle.current().pid());
        logMessage("Waiting for DLL injection...");
    }
    
    private void startTimer() {
        timer = new Timer(5000, new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                counter++;
                logMessage("Heartbeat #" + counter + " - " + new java.util.Date());
            }
        });
        timer.start();
    }
    
    private void logMessage(String message) {
        SwingUtilities.invokeLater(() -> {
            logArea.append("[" + new java.text.SimpleDateFormat("HH:mm:ss").format(new java.util.Date()) + "] " + message + "\n");
            logArea.setCaretPosition(logArea.getDocument().getLength());
        });
    }
    
    private void showSystemInfo() {
        StringBuilder info = new StringBuilder();
        info.append("=== System Information ===\n");
        info.append("Java Version: ").append(System.getProperty("java.version")).append("\n");
        info.append("Java Vendor: ").append(System.getProperty("java.vendor")).append("\n");
        info.append("OS Name: ").append(System.getProperty("os.name")).append("\n");
        info.append("OS Version: ").append(System.getProperty("os.version")).append("\n");
        info.append("Architecture: ").append(System.getProperty("os.arch")).append("\n");
        info.append("Available Processors: ").append(Runtime.getRuntime().availableProcessors()).append("\n");
        info.append("Max Memory: ").append(Runtime.getRuntime().maxMemory() / 1024 / 1024).append(" MB\n");
        info.append("Total Memory: ").append(Runtime.getRuntime().totalMemory() / 1024 / 1024).append(" MB\n");
        info.append("Free Memory: ").append(Runtime.getRuntime().freeMemory() / 1024 / 1024).append(" MB\n");
        info.append("========================");
        
        logMessage(info.toString());
    }
    
    public static void main(String[] args) {
        // 设置Look and Feel
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeel());
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        // 创建并显示应用程序
        SwingUtilities.invokeLater(() -> {
            new TargetApp().setVisible(true);
        });
        
        // 保持主线程运行
        System.out.println("Target Application is running...");
        System.out.println("Process ID: " + ProcessHandle.current().pid());
        System.out.println("Use the injector to inject JAR files into this process.");
    }
}