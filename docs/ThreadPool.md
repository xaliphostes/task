[< Back](index.md)

# ThreadPool

## Overview

The `ThreadPool` class is a specialized Algorithm that manages a collection of `Runnable` tasks and executes them in parallel. It provides an efficient way to distribute workloads across multiple threads, maximizing resource utilization while maintaining a clean interface for monitoring execution progress and managing task lifecycles.

## Features

- **Parallel Execution**: Runs multiple tasks concurrently across available CPU cores
- **Task Management**: Provides methods to add, create, and manage tasks
- **Execution Control**: Allows stopping all running tasks at once
- **Dynamic Task Creation**: Template method for creating and adding tasks in one step
- **Hardware Awareness**: Automatically detects optimal thread count for the system
- **Signal Forwarding**: Centralizes logging and monitoring from all managed tasks
- **Execution Statistics**: Reports information about execution time and task performance

## Class Interface

```cpp
class ThreadPool : public Algorithm {
public:
    // Constructor & destructor
    explicit ThreadPool(bool verbose = true);
    ~ThreadPool() = default;
    
    // Task management
    void add(std::unique_ptr<Runnable> runnable);
    
    template <typename T, typename... Args>
    T* createAndAdd(Args&&... args);
    
    // Information methods
    unsigned int size() const;
    static unsigned int maxThreadCount();
    
    // Execute all tasks in the pool
    void exec(const ArgumentPack &args = {}) override;
    
    // Utility methods
    void connectLoggerToAll(Task* logger);
    void stopAll();
    
protected:
    // Access to managed runnables
    const std::vector<std::unique_ptr<Runnable>>& runnables() const;
    
private:
    std::vector<std::unique_ptr<Runnable>> m_runnables;
    bool m_verbose;
};
```

## Signals

ThreadPool emits the following signals:

| Signal | Type | Description | Arguments |
|--------|------|-------------|-----------|
| `started` | Simple | Indicates pool execution has started | None |
| `finished` | Simple | Indicates pool execution has completed | None |
| `progress` | Data | Reports overall execution progress | float (0.0 to 1.0) |
| `stats` | Data | Reports execution statistics | int64_t (execution time in ms), unsigned int (number of tasks) |
| `log`, `warn`, `error` | Data | Forwarded log messages from tasks | std::string (message) |

## Usage Example

```cpp
// Create a thread pool
auto pool = std::make_shared<ThreadPool>();

// Create a logger and connect it to the pool
auto logger = std::make_shared<Logger>("PoolLogger");
logger->connectAllSignalsTo(pool.get());

// Create and add tasks using the template method
auto task1 = pool->createAndAdd<MyTask>("Task1", 1000);
auto task2 = pool->createAndAdd<MyTask>("Task2", 2000);
auto task3 = pool->createAndAdd<MyTask>("Task3", 1500);

// Alternatively, create and add tasks separately
auto task4 = std::make_unique<MyTask>("Task4", 3000);
pool->add(std::move(task4));

// Connect to statistics signal
pool->connectData("stats", [](const ArgumentPack& args) {
    int64_t executionTime = args.get<int64_t>(0);
    unsigned int taskCount = args.get<unsigned int>(1);
    
    std::cout << "Executed " << taskCount << " tasks in " 
              << executionTime << " ms ("
              << static_cast<double>(executionTime) / taskCount 
              << " ms per task average)" << std::endl;
});

// Execute all tasks in parallel
pool->exec();

// Alternative: execute asynchronously
auto future = pool->run();

// Do other work while tasks are running

// Check if we need to stop tasks
if (userRequestedStop) {
    pool->stopAll();
}

// Wait for completion
future.wait();
```

## How It Works

The ThreadPool operates through the following mechanisms:

1. **Task Collection**:
   - Tasks are added to the pool via `add()` or `createAndAdd<T>()`
   - All tasks must inherit from `Runnable` to provide a consistent interface

2. **Parallel Execution**:
   - When `exec()` is called, the pool starts all tasks using `std::async`
   - Each task runs in its own thread, with the system managing thread distribution
   - The pool waits for all tasks to complete using futures

3. **Progress Tracking**:
   - Overall progress is calculated as tasks complete
   - Each completed task contributes equally to overall progress

4. **Signal Forwarding**:
   - Log messages from tasks are forwarded to pool listeners
   - This allows centralized logging and monitoring

5. **Task Management**:
   - The pool maintains ownership of tasks via unique pointers
   - Tasks remain valid until the pool is destroyed

## Thread Management

The ThreadPool handles thread creation and management automatically:

- **Thread Count**: By default, uses as many threads as there are available hardware cores
- **Thread Creation**: Uses `std::async` with `std::launch::async` policy for automatic thread management
- **Thread Safety**: All tasks should be thread-safe as they will execute concurrently

The `maxThreadCount()` static method returns the number of hardware threads available on the system, which is typically used as the maximum number of concurrent tasks.

## Task Ownership

The ThreadPool takes ownership of tasks:

- Tasks are stored as `std::unique_ptr<Runnable>` in the pool
- The `createAndAdd<T>()` method returns a raw pointer for convenience
- Tasks are automatically destroyed when the pool is destroyed
- Do not delete raw pointers returned by `createAndAdd<T>()`

## Stopping Tasks

The ThreadPool provides a way to stop all running tasks:

```cpp
// Stop all tasks in the pool
pool->stopAll();
```

This calls `requestStop()` on all runnables in the pool. Tasks should check `stopRequested()` regularly and exit gracefully when requested.

## Logger Connection

The `connectLoggerToAll()` method provides a convenient way to connect a logger to all tasks in the pool:

```cpp
// Connect a logger to all tasks
auto logger = std::make_shared<Logger>();
pool->connectLoggerToAll(logger.get());
```

This ensures that log messages from all tasks are properly captured and processed.

## Performance Considerations

- **Task Granularity**: For optimal performance, tasks should be sufficiently large to offset the overhead of thread creation
- **CPU vs. I/O**: The pool works best with CPU-bound tasks; I/O-bound tasks might benefit from a larger pool size
- **Thread Count**: Using more threads than CPU cores typically does not improve performance for CPU-bound tasks
- **Task Dependencies**: Tasks should be independent; dependent tasks should be managed separately

## Best Practices

1. **Task Independence**: Design tasks to be independent and avoid shared state
2. **Error Handling**: Implement proper error handling in tasks to prevent pool execution failures
3. **Resource Management**: Ensure tasks properly manage resources even when stopped
4. **Progress Reporting**: Implement progress reporting in tasks for accurate pool progress
5. **Appropriate Sizing**: Create an appropriate number of tasks based on workload characteristics
6. **Task Granularity**: Balance task size to minimize threading overhead while maximizing parallelism

## Integration with Other Components

ThreadPool works well with other task library components:

- **Chronometer**: Measure detailed execution time of the pool and individual tasks
- **Logger**: Centralize logging from all tasks in the pool
- **TaskObserver**: Monitor performance metrics across the pool
- **ProgressMonitor**: Track overall progress of complex operations

## Implementation Details

- Uses `std::future` for task execution and coordination
- Uses `std::chrono` for execution time measurement
- Forwards signals using lambda functions
- Tasks are executed in the order they were added, but may complete in any order
- The `verbose` parameter controls whether execution statistics are automatically logged