[< Back](index.md)

# TaskQueue

## Overview

The `TaskQueue` class provides a priority-based task scheduling system for the task library. It enables executing tasks based on their importance level while managing concurrency to optimize system resource utilization. The queue ensures that high-priority tasks execute before lower-priority ones, regardless of their enqueue order.

## Features

- **Priority-Based Scheduling**: Tasks are executed according to their priority level
- **Concurrency Control**: Limits the number of simultaneously running tasks
- **Asynchronous Processing**: Runs tasks in separate threads managed by the queue
- **Robust Error Handling**: Captures and reports task failures
- **Comprehensive Status Reporting**: Emits signals for task lifecycle events
- **Dynamic Configuration**: Allows adjusting maximum concurrency during runtime
- **Graceful Shutdown**: Supports clean stopping with optional waiting for tasks

## Class Interface

```cpp
enum class TaskPriority {
    CRITICAL = 0,  // Highest priority, execute immediately
    HIGH = 1,      // High priority tasks
    NORMAL = 2,    // Normal priority tasks
    LOW = 3,       // Low priority, can wait
    BACKGROUND = 4 // Lowest priority, run when system is idle
};

class TaskQueue : public Task {
public:
    // Constructor & Destructor
    TaskQueue(unsigned int maxConcurrentTasks = 1, bool autoStart = true);
    ~TaskQueue();
    
    // Queue control
    void start();
    void stop(bool wait = true);
    void stopAll();
    
    // Task management
    bool enqueue(std::shared_ptr<Runnable> task, 
                 TaskPriority priority = TaskPriority::NORMAL,
                 const std::string& description = "");
    
    template <typename T, typename... Args>
    std::shared_ptr<T> createAndEnqueue(TaskPriority priority, 
                                         const std::string& description, 
                                         Args&&... args);
    
    size_t clearQueue();
    
    // Status methods
    size_t pendingCount() const;
    size_t activeCount() const;
    bool isRunning() const;
    
    // Configuration
    void setMaxConcurrentTasks(unsigned int maxTasks);
    unsigned int getMaxConcurrentTasks() const;
};
```

## Signals

TaskQueue emits the following signals:

| Signal          | Description                        | Arguments                                     |
| --------------- | ---------------------------------- | --------------------------------------------- |
| `taskEnqueued`  | When a task is added to the queue  | taskDescription, priorityValue                |
| `taskStarted`   | When a task begins execution       | taskDescription, priorityValue                |
| `taskCompleted` | When a task completes successfully | taskDescription, priorityValue                |
| `taskFailed`    | When a task encounters an error    | taskDescription, priorityValue, errorMessage  |
| `queueStats`    | When queue statistics are updated  | pendingCount, activeCount, maxConcurrentTasks |

## Usage Example

```cpp
// Create a task queue with 3 concurrent tasks
auto taskQueue = std::make_shared<TaskQueue>(3);

// Connect to queue signals
taskQueue->connectData("taskCompleted", [](const ArgumentPack& args) {
    std::string taskName = args.get<std::string>(0);
    std::cout << "Task completed: " << taskName << std::endl;
});

// Create and enqueue tasks with different priorities
auto criticalTask = std::make_shared<MyTask>("Critical operation", 1000);
taskQueue->enqueue(criticalTask, TaskPriority::CRITICAL, "Critical Task");

auto normalTask = std::make_shared<MyTask>("Normal operation", 2000);
taskQueue->enqueue(normalTask, TaskPriority::NORMAL, "Normal Task");

// Create and enqueue in one step
auto ioTask = taskQueue->createAndEnqueue<IoTask>(
    TaskPriority::LOW, 
    "Background IO Task",
    "IO operation", 
    3000
);

// Wait for completion
while (taskQueue->activeCount() > 0 || taskQueue->pendingCount() > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Active: " << taskQueue->activeCount() 
              << ", Pending: " << taskQueue->pendingCount() << std::endl;
}

// Change concurrency limit dynamically
taskQueue->setMaxConcurrentTasks(5);

// Stop the queue when done
taskQueue->stop();
```

## Priority Levels

TaskQueue supports five priority levels:

1. **CRITICAL (0)**: Highest priority, for tasks that must execute immediately
2. **HIGH (1)**: Important tasks that should run before normal operations
3. **NORMAL (2)**: Standard tasks with no special priority considerations
4. **LOW (3)**: Tasks that can wait if the system is busy
5. **BACKGROUND (4)**: Lowest priority, only execute when system is idle

Tasks are executed in order of priority, with CRITICAL tasks always executing before HIGH tasks, and so on. Within the same priority level, tasks are executed in FIFO (First-In, First-Out) order.

## How It Works

The TaskQueue operates using the following mechanisms:

1. **Task Submission**:
   - When a task is enqueued, it's wrapped in a TaskEntry structure with priority information
   - The task is inserted into a priority queue that orders tasks by priority and submission time

2. **Processing Thread**:
   - A dedicated thread monitors the priority queue
   - When capacity is available (active tasks < max concurrent), it dequeues the highest priority task
   - The task is executed in its own thread, detached from the queue processor

3. **Concurrency Management**:
   - The queue tracks active task count and enforces the maximum concurrent limit
   - When tasks complete, the count is decremented and the processor checks for new tasks to start

4. **Signal Forwarding**:
   - The queue forwards important signals from tasks to its own listeners
   - This allows centralized monitoring of all tasks in the queue

## Thread Safety

TaskQueue is designed to be thread-safe:

- Access to the internal queue is protected by a mutex
- Active task counting uses atomic variables
- Thread synchronization uses condition variables
- Task execution happens in separate threads

Multiple threads can safely call `enqueue()` and other methods concurrently.

## Use Cases

TaskQueue is particularly useful for:

1. **UI Applications**: Prioritizing UI responsiveness over background work
2. **Server Applications**: Managing request processing with different SLA requirements
3. **Resource-Constrained Systems**: Limiting concurrent operations to prevent overload
4. **Batch Processing**: Managing multiple jobs with different importance levels
5. **Data Processing Pipelines**: Coordinating various processing stages

## Implementation Details

- Uses a priority queue with a custom comparator for task ordering
- Employs mutexes and condition variables for thread synchronization
- Leverages atomic variables for thread-safe state tracking
- Forwards task signals to queue listeners for centralized monitoring
- Maintains collections of pending and active tasks
- Processes tasks asynchronously in separate threads