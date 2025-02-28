[< Back](index.md)

# Algorithm

## Overview

The `Algorithm` class is a core component of the task library that provides a framework for implementing executable algorithms with standardized signaling, progress reporting, and execution control. It extends the base `Task` class to add algorithm-specific functionality including execution state tracking, cancellation support, and dirty state management.

## Features

- **Standardized Execution**: Common interface for algorithm implementation and execution
- **Progress Reporting**: Built-in mechanism for reporting execution progress
- **Cancellation Support**: Ability to request and detect stop requests during execution
- **State Management**: Tracking of "dirty" state to determine if re-execution is needed
- **Asynchronous Execution**: Support for running algorithms in background threads
- **Signal-Based Communication**: Events for algorithm lifecycle (started, finished, errors)
- **Execution Arguments**: Flexible argument passing mechanism via `ArgumentPack`

## Class Interface

```cpp
class Algorithm : public Task {
public:
    // Constructor & destructor
    Algorithm();
    virtual ~Algorithm() = default;
    
    // Core algorithm execution method (must be implemented by subclasses)
    virtual void exec(const ArgumentPack &args = {}) = 0;
    
    // Execution control
    bool stopRequested() const;
    void stop();
    bool isRunning() const;
    bool isDirty() const;
    void setDirty(bool dirty);
    
    // Progress reporting
    void reportProgress(float progress);
    
    // Asynchronous execution
    std::future<void> run(const ArgumentPack &args = {});
    
protected:
    // Internal implementation of run
    void runImpl(const ArgumentPack &args);
    
private:
    bool m_dirty;
    bool m_stopRequested;
    bool m_isRunning;
};
```

## Signals

Algorithm emits the following signals:

| Signal     | Type   | Description                                 | Arguments                   |
| ---------- | ------ | ------------------------------------------- | --------------------------- |
| `progress` | Data   | Reports execution progress                  | float (0.0 to 1.0)          |
| `started`  | Simple | Indicates algorithm has started execution   | None                        |
| `finished` | Simple | Indicates algorithm has completed execution | None                        |
| `error`    | Data   | Reports execution errors                    | std::string (error message) |

## Usage Example

```cpp
// Create a custom algorithm by extending the Algorithm class
class DataProcessor : public Algorithm {
public:
    DataProcessor() {
        // Create additional signals if needed
        createDataSignal("result");
    }
    
    void setData(const std::vector<double>& data) {
        m_data = data;
        setDirty(true); // Mark as needing execution
    }
    
    // Implement the required exec method
    void exec(const ArgumentPack& args = {}) override {
        if (m_data.empty()) {
            emitString("error", "No data to process");
            return;
        }
        
        double sum = 0.0;
        double min = m_data[0];
        double max = m_data[0];
        
        // Process data with progress reporting
        for (size_t i = 0; i < m_data.size(); ++i) {
            // Check for cancellation
            if (stopRequested()) {
                emitString("warn", "Processing stopped by user");
                return;
            }
            
            // Process this data point
            double value = m_data[i];
            sum += value;
            min = std::min(min, value);
            max = std::max(max, value);
            
            // Report progress
            float progress = static_cast<float>(i + 1) / m_data.size();
            reportProgress(progress);
        }
        
        // Calculate final results
        double mean = sum / m_data.size();
        
        // Emit results
        ArgumentPack resultArgs;
        resultArgs.add<double>(min);
        resultArgs.add<double>(max);
        resultArgs.add<double>(mean);
        emit("result", resultArgs);
        
        emitString("log", "Data processing completed successfully");
    }
    
private:
    std::vector<double> m_data;
};

// Using the algorithm
void processData() {
    // Create the algorithm
    auto processor = std::make_shared<DataProcessor>();
    
    // Set data to process
    std::vector<double> data = {1.2, 3.4, 5.6, 7.8, 9.0};
    processor->setData(data);
    
    // Connect to signals
    processor->connectData("result", [](const ArgumentPack& args) {
        double min = args.get<double>(0);
        double max = args.get<double>(1);
        double mean = args.get<double>(2);
        
        std::cout << "Results: min=" << min << ", max=" << max 
                  << ", mean=" << mean << std::endl;
    });
    
    processor->connectData("progress", [](const ArgumentPack& args) {
        float progress = args.get<float>(0);
        std::cout << "Progress: " << (progress * 100) << "%" << std::endl;
    });
    
    // Execute asynchronously
    auto future = processor->run();
    
    // Wait for completion if needed
    future.wait();
}
```

## Lifecycle

An Algorithm goes through the following lifecycle:

1. **Construction**: Internal state initialized (dirty=true, running=false, stopRequested=false)
2. **Configuration**: Algorithm-specific parameters set by calling its methods
3. **Execution Start**: `run()` or `runImpl()` called, emits "started" signal
4. **Execution**: `exec()` implementation runs, may report progress
5. **Execution End**: Algorithm completes, emits "finished" signal
6. **Error Handling**: If an exception occurs, emits "error" signal with details

This lifecycle can be repeated multiple times for the same algorithm instance.

## The Dirty State

The Algorithm class maintains a "dirty" state that indicates whether the algorithm needs to be re-executed:

- `isDirty()`: Returns true if the algorithm's input has changed since last execution
- `setDirty(bool)`: Explicitly sets the dirty state

This mechanism is useful for:
- Avoiding unnecessary re-execution
- Forcing re-execution when needed
- Tracking when inputs have changed

Setting dirty to true while an algorithm is running causes a stop request to be issued.

## Asynchronous Execution

The `run()` method provides asynchronous execution using C++ futures:

```cpp
// Start algorithm in a background thread
std::future<void> future = algorithm->run();

// Do other work while algorithm executes

// Wait for completion when needed
future.wait();
```

This allows algorithms to run in the background without blocking the main thread.

## Cancellation Mechanism

Algorithms can be cancelled during execution:

1. **Requesting Cancellation**: Call `stop()` to set the stop flag
2. **Checking for Cancellation**: Algorithm should periodically call `stopRequested()`
3. **Handling Cancellation**: If stop is requested, algorithm can clean up and exit

Example of handling cancellation in an algorithm:

```cpp
void exec(const ArgumentPack& args) override {
    for (int i = 0; i < 100; i++) {
        // Check for cancellation frequently
        if (stopRequested()) {
            emitString("log", "Operation cancelled");
            return; // Exit early
        }
        
        // Perform work step
        // ...
        
        // Report progress
        reportProgress(static_cast<float>(i) / 100.0f);
    }
}
```

## Best Practices

1. **Check Stop Requests**: Call `stopRequested()` regularly during long-running operations
2. **Report Progress**: Use `reportProgress()` to provide feedback on execution status
3. **Handle Exceptions**: Catch and report exceptions specific to your algorithm
4. **Manage Dirty State**: Set dirty=true when inputs change to indicate re-execution is needed
5. **Use Run for Background Execution**: Use `run()` for asynchronous execution rather than calling `exec()` directly
6. **Define Clear Signals**: Create algorithm-specific signals for returning results
7. **Clean Up Resources**: Ensure resources are released properly, especially if cancellation occurs

## Integration with Other Components

Algorithm works well with other task library components:

- **ThreadPool**: Execute multiple algorithms in parallel
- **TaskQueue**: Schedule algorithms with different priorities
- **TaskObserver**: Monitor algorithm execution statistics
- **ProgressMonitor**: Track progress across multiple algorithms
- **Chronometer**: Measure algorithm execution time

## Implementation Details

- Uses `std::future` and `std::async` for asynchronous execution
- Mutex protection is not used for state flags since they're accessed atomically
- The `exec()` method should be thread-safe as it may be called from multiple threads
- Progress values should be between 0.0 and 1.0 (0-100%)
- Exceptions in `exec()` are caught and reported via the error signal