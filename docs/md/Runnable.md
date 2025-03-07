[< Back](index.md)

# Runnable

## Overview

The `Runnable` class is a key component of the task library that provides a standardized interface for executable tasks. It extends the base `Task` class to add execution state tracking, progress reporting, and cancellation support. Runnable serves as the foundation for tasks that can be executed by the ThreadPool or independently.

## Features

- **Standardized Execution Interface**: Common interface for all executable tasks
- **Execution State Tracking**: Methods to check if a task is running or has been requested to stop
- **Progress Reporting**: Built-in mechanism for reporting task completion percentage
- **Cancellation Support**: Ability to request and detect stop requests during execution
- **Asynchronous Execution**: Support for running tasks in background threads
- **Signal-Based Communication**: Pre-defined signals for task lifecycle (started, finished, progress)

## Class Interface

```cpp
class Runnable : public Task {
public:
    // Constructor & destructor
    Runnable();
    virtual ~Runnable() = default;
    
    // Task execution
    void run();
    std::future<void> runAsync();
    
    // State methods
    bool isRunning() const;
    void requestStop();
    bool stopRequested() const;
    
protected:
    // Implementation method (must be overridden by subclasses)
    virtual void runImpl() = 0;
    
    // Progress reporting helper
    void reportProgress(float progress);
    
private:
    bool m_isRunning;
    bool m_stopRequested;
};
```

## Signals

Runnable emits the following signals:

| Signal                 | Type   | Description                            | Arguments             |
| ---------------------- | ------ | -------------------------------------- | --------------------- |
| `started`              | Simple | Emitted when the task begins execution | None                  |
| `finished`             | Simple | Emitted when the task completes        | None                  |
| `progress`             | Data   | Reports task completion percentage     | float (0.0 to 1.0)    |
| `log`, `warn`, `error` | Data   | Standard logging signals               | std::string (message) |

## Usage Example

```cpp
// Create a custom task by extending Runnable
class DataProcessor : public Runnable {
public:
    DataProcessor(const std::string& name, const std::vector<int>& data)
        : m_name(name), m_data(data) {}
    
protected:
    // Implement the required runImpl method
    void runImpl() override {
        emitString("log", m_name + " started processing " + 
                  std::to_string(m_data.size()) + " items");
        
        int sum = 0;
        
        // Process each item with progress updates
        for (size_t i = 0; i < m_data.size(); ++i) {
            // Check if we should stop
            if (stopRequested()) {
                emitString("warn", m_name + " stopped before completion");
                return;
            }
            
            // Process this item
            sum += m_data[i];
            
            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Report progress (0.0 to 1.0)
            float progress = static_cast<float>(i + 1) / m_data.size();
            reportProgress(progress);
            
            // Log milestone progress
            if ((i + 1) % (m_data.size() / 10) == 0 || i + 1 == m_data.size()) {
                emitString("log", m_name + " processed " + 
                          std::to_string(i + 1) + "/" + 
                          std::to_string(m_data.size()) + " items");
            }
        }
        
        // Report result
        emitString("log", m_name + " completed with sum: " + std::to_string(sum));
    }
    
private:
    std::string m_name;
    std::vector<int> m_data;
};

// Using the task
void processData() {
    // Create the task
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto processor = std::make_shared<DataProcessor>("Processor1", data);
    
    // Connect to signals
    processor->connectSimple("started", []() {
        std::cout << "Processing started" << std::endl;
    });
    
    processor->connectSimple("finished", []() {
        std::cout << "Processing finished" << std::endl;
    });
    
    processor->connectData("progress", [](const ArgumentPack& args) {
        float progress = args.get<float>(0);
        std::cout << "Progress: " << (progress * 100) << "%" << std::endl;
    });
    
    // Execute asynchronously
    std::future<void> future = processor->runAsync();
    
    // Do other work while task is running
    
    // Wait for completion
    future.wait();
}
```

## Life Cycle

A Runnable task goes through the following lifecycle:

1. **Construction**: Internal state initialized (running=false, stopRequested=false)
2. **Configuration**: Task-specific parameters set by calling its methods
3. **Execution Start**: `run()` or `runAsync()` called, sets running=true, emits "started" signal
4. **Task Execution**: `runImpl()` implementation runs, may report progress
5. **Execution End**: Task completes, sets running=false, emits "finished" signal
6. **Error Handling**: If an exception occurs, emits "error" signal with details

This lifecycle can be repeated multiple times for the same task instance, as long as a task is not already running when `run()` is called.

## Progress Reporting

The `reportProgress()` method allows tasks to report their completion percentage:

```cpp
// Report 50% completion
reportProgress(0.5f);
```

Progress values should be between 0.0 (0% complete) and 1.0 (100% complete).

The progress value is emitted through the "progress" signal, allowing other components to track and display task progress.

## Asynchronous Execution

The `runAsync()` method provides asynchronous execution using C++ futures:

```cpp
// Start task in a background thread
std::future<void> future = task->runAsync();

// Do other work while task executes

// Wait for completion when needed
future.wait();
```

This allows tasks to run in the background without blocking the main thread.

## Cancellation Mechanism

Runnable tasks support cancellation during execution:

1. **Requesting Cancellation**: Call `requestStop()` to set the stop flag
2. **Checking for Cancellation**: Task should periodically call `stopRequested()`
3. **Handling Cancellation**: If stop is requested, task can clean up and exit

Example of handling cancellation in a task:

```cpp
void runImpl() override {
    for (int i = 0; i < 100; i++) {
        // Check for cancellation frequently
        if (stopRequested()) {
            emitString("log", "Task cancelled");
            return; // Exit early
        }
        
        // Perform work step
        // ...
        
        // Report progress
        reportProgress(static_cast<float>(i) / 100.0f);
    }
}
```

## ThreadPool Integration

Runnable tasks are designed to work seamlessly with the ThreadPool:

```cpp
// Create a thread pool
auto pool = std::make_shared<ThreadPool>();

// Add tasks to the pool
pool->add(std::make_unique<MyTask>());
pool->add(std::make_unique<AnotherTask>());

// Create and add a task in one step
auto task = pool->createAndAdd<DataProcessor>("Processor", data);

// Execute all tasks in parallel
pool->exec();
```

The ThreadPool manages the execution of all tasks, collecting results and providing centralized monitoring.

## Error Handling

Runnable implements automatic error handling:

```cpp
void run() {
    try {
        // Run the implementation
        runImpl();
    } catch (const std::exception& e) {
        // Report error if an exception occurs
        emitString("error", e.what());
    } catch (...) {
        // Handle unknown exceptions
        emitString("error", "Unknown exception during task execution");
    }
}
```

This ensures that exceptions in tasks are properly captured and reported, preventing crashes and allowing for proper error handling.

## Thread Safety

Important thread safety considerations for Runnable tasks:

- **State Flags**: The `m_isRunning` and `m_stopRequested` flags are accessed atomically
- **Thread Confinement**: `runImpl()` executes in a single thread
- **Shared Resources**: Tasks should properly synchronize access to shared resources
- **Signal Emission**: Signal emission is thread-safe
- **Multiple Instances**: Different instances of the same Runnable subclass can run concurrently

## Best Practices

1. **Check Stop Requests**: Call `stopRequested()` regularly during long-running operations
2. **Report Progress**: Use `reportProgress()` to provide feedback on execution status
3. **Clean Resources**: Ensure resources are properly released, especially if cancellation occurs
4. **Handle Exceptions**: Catch and handle task-specific exceptions in `runImpl()`
5. **Avoid Blocking**: Minimize blocking operations that could prevent cancellation
6. **Thread Safety**: Ensure thread safety when accessing shared resources

## Implementation Details

- Uses `std::future` and `std::async` for asynchronous execution
- Progress values are automatically clamped to the range [0.0, 1.0]
- The `run()` method checks if the task is already running to prevent concurrent execution
- Exceptions in `runImpl()` are caught and reported via the error signal
- Uses boolean flags to track execution state