# FileLogger Integration Guide

This guide demonstrates how to integrate the `FileLogger` component into your applications built with the Task framework. The `FileLogger` extends the framework's logging capabilities by adding persistent file-based logging with features like log rotation, formatting options, and multi-level logging.

![FileLogger Architecture](filelogger_diagram.png)

## Basic Setup

### 1. Include the header

```cpp
#include <task/FileLogger.h>
```

### 2. Create a configuration

The `FileLoggerConfig` structure provides numerous options to customize your logging:

```cpp
FileLoggerConfig config;
config.logDirectory = "logs";                  // Where to store log files
config.filenamePattern = "app_%Y%m%d.log";     // Supports date/time patterns
config.maxFileSize = 10 * 1024 * 1024;         // 10MB max size
config.maxFiles = 5;                           // Keep 5 most recent logs
config.includeTimestamps = true;               // Add timestamps to entries
config.includeLogLevel = true;                 // Show log level (INFO, ERROR, etc.)
```

### 3. Create the logger

```cpp
// Create with specific minimum log level (Debug, Info, Warning, Error, Fatal)
auto logger = std::make_shared<FileLogger>(config, LogLevel::Info);

// Or create with default settings
auto defaultLogger = std::make_shared<FileLogger>();
```

### 4. Log messages with levels

```cpp
// Log at specific levels
logger->logWithLevel(LogLevel::Debug, "Detailed debugging information");
logger->logWithLevel(LogLevel::Info, "General information message");
logger->logWithLevel(LogLevel::Warning, "Warning about a potential issue");
logger->logWithLevel(LogLevel::Error, "An error has occurred");
logger->logWithLevel(LogLevel::Fatal, "Fatal error, cannot continue");
```

## Integration with Task Components

The `FileLogger` integrates seamlessly with other Task framework components through the signal-slot system:

### Connect to an Algorithm

```cpp
auto algorithm = std::make_shared<MyAlgorithm>();
logger->connectAllSignalsTo(algorithm.get());

// Now all logs, warnings, and errors from the algorithm will be captured
algorithm->exec();
```

### Connect to a ThreadPool

```cpp
auto threadPool = std::make_shared<ThreadPool>();
logger->connectAllSignalsTo(threadPool.get());

// Add tasks to the pool
threadPool->createAndAdd<MyTask>("Task1");
threadPool->createAndAdd<MyTask>("Task2");

// Execute and log all activity
threadPool->exec();
```

### Connect to multiple components

```cpp
// Create components
auto chrono = std::make_shared<Chronometer>();
auto counter = std::make_shared<Counter>();
auto algorithm = std::make_shared<MyAlgorithm>();

// Connect all to the same logger
std::vector<Task*> tasks = {chrono.get(), counter.get(), algorithm.get()}

## Performance Considerations

When integrating `FileLogger` into your applications, consider these performance optimization strategies:

### Logging Level Selection

Choose appropriate log levels for different environments:

- **Development**: Use `LogLevel::Debug` for maximum information
- **Testing**: Use `LogLevel::Info` for general operational details
- **Production**: Use `LogLevel::Warning` or higher to reduce I/O overhead

### File I/O Optimization

File operations can impact performance:

```cpp
// For high-throughput scenarios
FileLoggerConfig config;
config.flushAfterEachWrite = false;  // Reduces I/O but logs might not be immediately persisted
config.maxFileSize = 20 * 1024 * 1024;  // Larger files mean fewer rotations
```

### Memory Usage

Manage memory usage with appropriate buffer settings:

```cpp
// For high-volume logging systems
std::cout.sync_with_stdio(false);  // Improve console output performance
```

### Thread Safety

The `FileLogger` is thread-safe by design, but consider these strategies:

- Use one centralized logger for the application to avoid excessive lock contention
- For extremely high-volume logging, consider separate loggers for different subsystems
- Use the log level filtering to reduce unnecessary file operations

## Troubleshooting

### Common Issues

1. **No log files being created**
   - Check that the log directory exists or enable `createDirectoryIfNotExists`
   - Verify the process has write permissions to the directory
   - Check the disk space availability

2. **Log files not containing expected entries**
   - Verify that the minimum log level is appropriate
   - Ensure signal connections are properly established
   - Check for errors in the console output

3. **Excessive log file generation**
   - Adjust `maxFileSize` to a larger value
   - Adjust `maxFiles` to manage disk usage
   - Consider a more restrictive log level in production

### Debugging File Logger Issues

```cpp
// Create a debug-level console logger to debug the file logger itself
auto debugLogger = std::make_shared<Logger>("FileLoggerDebug");

// Connect file logger error signals to the debug logger
fileLogger->connectData("fileError", debugLogger.get(), &Logger::error);
fileLogger->connectSimple("fileRotated", [&debugLogger]() {
    ArgumentPack args;
    args.add<std::string>("File rotation occurred");
    debugLogger->log(args);
});
```

## Conclusion

The `FileLogger` component provides a powerful, flexible logging solution that integrates seamlessly with the Task framework. By leveraging its features like log rotation, level filtering, and signal-based integration, you can implement comprehensive logging throughout your application with minimal effort.

For more details, refer to the complete [FileLogger API documentation](api_reference.html) and check out the [example applications](examples.html)

## Common Logging Patterns

### Task Lifecycle Logging

Track the complete lifecycle of tasks:

```cpp
// Create a custom algorithm
class MyAlgorithm : public Algorithm {
public:
    MyAlgorithm() {
        // You can optionally create a named logger for this component
        m_componentLogger = std::make_shared<FileLogger>(
            FileLoggerConfig{}, LogLevel::Debug, "MyAlgorithm");
    }
    
    void exec(const ArgumentPack &args = {}) override {
        // Log start with component logger (if using)
        if (m_componentLogger) {
            m_componentLogger->logWithLevel(LogLevel::Info, "Algorithm starting execution");
        }
        
        // Or use signal emission (picked up by any connected logger)
        emitString("log", "Beginning execution phase 1");
        
        // Normal execution code...
        reportProgress(0.5f);
        
        emitString("log", "Beginning execution phase 2");
        
        // Complete execution
        emitString("log", "Algorithm execution completed");
    }
    
private:
    std::shared_ptr<FileLogger> m_componentLogger;
};
```

### Performance Logging

Track and log performance metrics:

```cpp
// Log execution time
chrono->start();
algorithm->exec();
auto elapsed = chrono->stop();

logger->logWithLevel(LogLevel::Info, 
    "Execution completed in " + std::to_string(elapsed) + " ms");

// Log memory usage
size_t memoryUsage = getCurrentMemoryUsage(); // Your memory tracking function
logger->logWithLevel(LogLevel::Debug, 
    "Memory usage: " + std::to_string(memoryUsage / 1024 / 1024) + " MB");
```

### Structured Logging

Create more structured log entries for easier parsing:

```cpp
// JSON-like structured logging
std::stringstream ss;
ss << "{"
   << "\"operation\":\"fileUpload\","
   << "\"fileName\":\"" << fileName << "\","
   << "\"fileSize\":" << fileSize << ","
   << "\"status\":\"" << status << "\","
   << "\"processingTimeMs\":" << processingTime
   << "}";
   
logger->logWithLevel(LogLevel::Info, ss.str(), "FileProcessor");
```

## Advanced Usage

### Log Rotation Callbacks

Register callbacks to be notified when log files are rotated:

```cpp
int callbackId = logger->registerRotationCallback([](const std::filesystem::path& oldPath) {
    std::cout << "Log rotated, old file: " << oldPath << std::endl;
    
    // Potentially archive or process the old log file
    // e.g., compress it or move it to long-term storage
});

// Later, if needed:
logger->unregisterRotationCallback(callbackId);
```

### Explicit Log Rotation

You can manually trigger log rotation:

```cpp
// Rotate the log file manually (e.g., at application startup)
logger->rotateLogFile();

// This creates a new log file and properly closes the old one
```

### Dynamic Configuration

Update configuration at runtime:

```cpp
FileLoggerConfig newConfig;
newConfig.maxFileSize = 5 * 1024 * 1024;  // 5MB
newConfig.flushAfterEachWrite = false;    // Improve performance
newConfig.includeTaskName = true;         // Add task name to entries
logger->updateConfig(newConfig);
```

### Changing Minimum Log Level

Adjust verbosity during runtime:

```cpp
// In development environments
logger->setMinimumLogLevel(LogLevel::Debug);

// In production environments
logger->setMinimumLogLevel(LogLevel::Warning);

// During troubleshooting
if (troubleshootingMode) {
    logger->setMinimumLogLevel(LogLevel::Debug);
} else {
    logger->setMinimumLogLevel(LogLevel::Info);
}
```

## Example: Comprehensive Application Logging

This example shows a more complete integration including exception handling and comprehensive logging:

```cpp
int main() {
    try {
        // Create and configure the logger
        FileLoggerConfig config;
        config.logDirectory = "logs";
        config.filenamePattern = "app_%Y%m%d_%H%M%S.log";
        auto logger = std::make_shared<FileLogger>(config);
        
        // Log application startup
        logger->logWithLevel(LogLevel::Info, "Application starting", "Main");
        
        // Create and connect components
        auto chrono = std::make_shared<Chronometer>();
        auto pool = std::make_shared<ThreadPool>();
        
        logger->connectAllSignalsTo(chrono.get());
        logger->connectAllSignalsTo(pool.get());
        
        // Log system information
        logger->logWithLevel(LogLevel::Info, 
            "Running with " + std::to_string(ThreadPool::maxThreadCount()) + " threads", 
            "System");
            
        // Add tasks to the pool
        for (int i = 0; i < 5; i++) {
            auto task = pool->createAndAdd<WorkerTask>("Worker-" + std::to_string(i), i*10);
            logger->connectAllSignalsTo(task);
        }
        
        // Execute with timing
        chrono->start();
        pool->exec();
        auto elapsed = chrono->stop();
        
        // Log completion
        logger->logWithLevel(LogLevel::Info, 
            "All tasks completed in " + std::to_string(elapsed) + " ms", 
            "Main");
            
        // Log application shutdown
        logger->logWithLevel(LogLevel::Info, "Application shutting down normally", "Main");
        return 0;
    }
    catch (const std::exception& e) {
        // Log any uncaught exceptions
        std::cerr << "FATAL: Uncaught exception: " << e.what() << std::endl;
        
        // Try to log to file if logger was created successfully
        try {
            if (logger) {
                logger->logWithLevel(LogLevel::Fatal, 
                    "Uncaught exception: " + std::string(e.what()), "Main");
            }
        } catch (...) {
            // Ignore errors during error logging
        }
        
        return 1;
    }
}
```
