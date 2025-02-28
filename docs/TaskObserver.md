[< Back](index.md)

# TaskObserver

## Overview

The `TaskObserver` class is a powerful monitoring tool for the task library that collects and analyzes statistics about task execution across the system. It provides insights into performance metrics, success rates, and execution patterns of tasks without requiring code changes to the monitored tasks.

## Features

- **Non-intrusive Monitoring**: Attaches to existing tasks using the signal-slot system
- **Comprehensive Statistics**: Tracks execution counts, timing, success/failure rates
- **Custom Metrics**: Supports application-specific metrics beyond the standard set
- **Performance Analysis**: Identifies slow tasks and execution bottlenecks
- **Status Reporting**: Generates detailed summary reports and real-time updates

## Class Interface

```cpp
class TaskObserver : public Task {
public:
    // Statistics structure for a single task
    struct TaskStats {
        std::string taskName;
        std::string taskType;
        int executionCount;
        int successCount;
        int failureCount;
        int64_t totalExecutionTimeMs;
        int64_t minExecutionTimeMs;
        int64_t maxExecutionTimeMs;
        std::chrono::system_clock::time_point lastExecutionTime;
        double lastProgress;
        std::map<std::string, double> customMetrics;
    };
    
    // Constructor
    explicit TaskObserver(const std::string& name = "TaskObserver");
    
    // Monitoring methods
    bool attach(Task* task, const std::string& taskName = "");
    bool detach(Task* task);
    
    // Statistics access
    const TaskStats* getTaskStats(Task* task) const;
    std::vector<TaskStats> getAllTaskStats() const;
    void resetAllStats();
    
    // Metrics management
    bool addCustomMetric(Task* task, const std::string& metricName, double value);
    
    // Analysis methods
    double getAverageExecutionTime(Task* task) const;
    double getSuccessRate(Task* task) const;
    std::string generateSummaryReport() const;
};
```

## Signals

TaskObserver emits the following signals:

| Signal         | Description                                          | Arguments                                                       |
| -------------- | ---------------------------------------------------- | --------------------------------------------------------------- |
| `statsUpdated` | Emitted when task statistics are updated             | taskName, taskType, execCount, successCount, failCount, avgTime |
| `taskStarted`  | Emitted when a monitored task begins execution       | taskName, taskType                                              |
| `taskFinished` | Emitted when a monitored task completes successfully | taskName, taskType, execTimeMs, success                         |
| `taskFailed`   | Emitted when a monitored task encounters an error    | taskName, taskType, errorMsg                                    |

## Usage Example

```cpp
// Create observer
auto observer = std::make_shared<TaskObserver>("MainObserver");

// Attach to tasks
observer->attach(myTask, "ImportantTask");
observer->attach(otherTask);  // Uses type name if no name provided

// Subscribe to observer events
observer->connectData("statsUpdated", [](const ArgumentPack& args) {
    std::string taskName = args.get<std::string>(0);
    int execCount = args.get<int>(2);
    double avgTime = args.get<double>(5);
    
    std::cout << "Task " << taskName << " stats updated: "
              << execCount << " executions, "
              << avgTime << "ms average time" << std::endl;
});

// Add custom metrics
observer->addCustomMetric(myTask, "MemoryUsageMB", 42.5);

// Get statistics
auto stats = observer->getTaskStats(myTask);
if (stats) {
    std::cout << "Success rate: " 
              << (static_cast<double>(stats->successCount) / stats->executionCount) * 100.0 
              << "%" << std::endl;
}

// Generate a report
std::string report = observer->generateSummaryReport();
std::cout << report << std::endl;
```

## How It Works

TaskObserver connects to task signals to monitor their lifecycle:

1. When `attach()` is called, the observer connects to the task's signals:
   - `started` - To detect when execution begins
   - `finished` - To detect successful completion
   - `error` - To detect failures
   - `progress` - To track progress updates

2. For each task event, the observer:
   - Updates internal statistics
   - Calculates derived metrics (e.g., average time, success rate)
   - Emits its own signals for monitoring

3. Applications can retrieve statistics at any time using the `getTaskStats()` or `getAllTaskStats()` methods.

## Benefits

- **Performance Optimization**: Identify slow tasks or bottlenecks
- **Reliability Monitoring**: Track failure rates across system components
- **Resource Management**: Understand execution patterns
- **System Tuning**: Gather data to optimize thread pool size or task priorities
- **Diagnostics**: Easily identify problematic tasks during development and testing

## Implementation Details

- Uses the existing signal-slot system for communication
- Maintains a map of task statistics by task pointer
- Tracks execution timing using `std::chrono`
- Stores weak references to connections to prevent memory leaks
- Thread-safe implementation with mutex protection for statistics access