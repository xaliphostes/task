# Task

<p align="center">
  <img src="https://img.shields.io/static/v1?label=Linux&logo=linux&logoColor=white&message=support&color=success" alt="Linux support">
  <img src="https://img.shields.io/static/v1?label=macOS&logo=apple&logoColor=white&message=support&color=success" alt="macOS support">
  <img src="https://img.shields.io/static/v1?label=Windows&logo=windows&logoColor=white&message=soon&color=red" alt="Windows support">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="Language">
  <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License">
</p>

A thread-safe, minimalist reactive library in C++

## Key Features
- **Thread-Safe Signals & Slots**: Robust communication between components in multi-threaded environments
- **Parallel Task Processing**: Efficient distribution of workloads across multiple threads
- **Reactive Programming Model**: Signal-based architecture for decoupled, event-driven applications
- **Progress Tracking**: Built-in mechanisms for monitoring and reporting execution progress
- **Comprehensive Task Management**: Execution control with support for cancellation and status reporting

## Requirements
- C++20 (with C++23 support coming soon)
- CMake

## Compilation
Create a `build` directory, **go inside** and type:
```sh
cmake .. && make -j12
```

## Running Unit Tests
**NOTE**: The internal CMake test framework is used to perform unit testing.

In the **same directory** as for the compilation (i.e., the `build` directory), type:
```sh
ctest
```
or
```sh
make test
```

## [Documentation in Markdown](docs/index.md)

## [Documentation in html](https://xaliphostes.github.io/task/)

## Thread-Safe SignalSlot System

The core of this library is the SignalSlot system, which now provides comprehensive thread safety guarantees:

- **Concurrent Signal Emission**: Multiple threads can safely emit signals simultaneously
- **Thread-Safe Connections**: Connect and disconnect operations are protected by mutex locks
- **Snapshot-Based Emissions**: Signal handlers are called on a snapshot to prevent iterator invalidation
- **Atomic Connection State**: Connection status is tracked with atomic variables
- **Synchronization Policies**: Control how signals are processed across threads

Example of thread-safe signal usage:

```cpp
// Thread-safe signal creation and connection
myComponent->createSignal("dataReady");
myComponent->connect("dataReady", [](const ArgumentPack& args) {
    // This handler can be safely called from any thread
    auto data = args.get<DataObject>(0);
    processData(data);
});

// Emit signals from any thread
myComponent->emit("dataReady", args);
```

## Custom Algorithm Example
This example demonstrates a comprehensive sensor data processing pipeline that leverages the thread-safe features of this library.

The example creates a robust pipeline for processing IoT sensor data that showcases the power of the enhanced task library. It safely handles concurrent processing of sensor data streams without race conditions or data corruption.

It also demonstrates important operational features like state persistence, performance profiling, and graceful shutdown handling.

This example showcases how the enhanced task library can be used to build sophisticated, resilient, multi-threaded data processing systems with minimal boilerplate code, allowing developers to focus on the business logic rather than concurrency infrastructure.

See this [**README**](examples/complete-example/README.md) in the `example/complete-example` folder

## Task Library Demo
The following example showcases a parallel data processing system with thread-safe components:

```cpp
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

// Include necessary headers for the task library
#include <task/Chronometer.h>
#include <task/Logger.h>
#include <task/ProgressMonitor.h>
#include <task/Task.h>
#include <task/concurrent/Runnable.h>
#include <task/concurrent/ThreadPool.h>

// Define a custom data processor task
class DataProcessor : public Runnable {
  public:
    DataProcessor(const std::string &name, int dataSize, int processingTime)
        : m_name(name), m_dataSize(dataSize), m_processingTime(processingTime) {
    }

  protected:
    void runImpl() override {
        emitString("log", m_name + " started processing " +
                              std::to_string(m_dataSize) + " data points");

        // Simulate processing with progress updates
        for (int i = 0; i < 10 && !stopRequested(); ++i) {
            // Simulate some work
            std::this_thread::sleep_for(
                std::chrono::milliseconds(m_processingTime / 10));

            // Report progress (0.0 to 1.0)
            float progress = static_cast<float>(i + 1) / 10.0f;
            reportProgress(progress);

            // Log progress milestone - thread-safe emission
            if ((i + 1) % 3 == 0) {
                emitString("log", m_name + " completed " +
                                      std::to_string((i + 1) * 10) +
                                      "% of processing");
            }
        }

        if (stopRequested()) {
            emitString("warn", m_name + " was stopped before completion");
        } else {
            emitString("log", m_name + " completed processing successfully");
        }
    }

  private:
    std::string m_name;
    int m_dataSize;
    int m_processingTime;
};

int main() {
    Logger logger("App");
    Chronometer chrono;
    ProgressMonitor progressMonitor;
    ThreadPool threadPool;

    // Thread-safe connection
    logger.connectAllSignalsTo(&chrono);
    logger.connectAllSignalsTo(&threadPool);

    // Create a random number generator for data sizes and processing times
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dataSizeDist(100, 1000);
    std::uniform_int_distribution<> processingTimeDist(200, 800);

    // Create and add processing tasks to the thread pool
    int numTasks = 5;
    progressMonitor.setTaskCount(numTasks);

    for (int i = 0; i < numTasks; i++) {
        std::string taskName = "Processor-" + std::to_string(i + 1);
        int dataSize = dataSizeDist(gen);
        int processingTime = processingTimeDist(gen);

        auto processor = threadPool.createAndAdd<DataProcessor>(
            taskName, dataSize, processingTime);

        // Thread-safe signal connections between processor and monitor
        processor->connect("started", &progressMonitor,
                          &ProgressMonitor::onTaskStarted);
        processor->connect("finished", &progressMonitor,
                          &ProgressMonitor::onTaskFinished);
        processor->connect("progress", &progressMonitor,
                          &ProgressMonitor::onProgress);

        // Connect logger to processor - thread-safe connections
        logger.connectAllSignalsTo(processor);
    }

    // Connect summary signal from monitor to a lambda that prints final stats
    progressMonitor.connect("summary", [](const ArgumentPack &args) {
        int totalTasks = args.get<int>(0);
        int completedTasks = args.get<int>(1);
        std::cout << "\n===== Summary =====\n";
        std::cout << "Total tasks: " << totalTasks << "\n";
        std::cout << "Completed tasks: " << completedTasks << "\n";
        std::cout << "===================" << std::endl;
    });

    // Start timing
    std::cout << "Starting parallel processing with " << numTasks << " tasks..."
              << std::endl;
    chrono.start();

    // Execute all tasks in the thread pool with thread-safe signal
    // communication
    threadPool.exec();

    // Stop timing and display results
    int64_t elapsed = chrono.stop();
    std::cout << "All tasks completed in " << elapsed << " ms" << std::endl;

    // Handle a stop request demonstration (for a new set of tasks)
    std::cout << "\nDemonstrating stop functionality:" << std::endl;

    // Create a new processor and add it to a new thread pool
    ThreadPool stopDemoPool;
    auto longProcessor =
        stopDemoPool.createAndAdd<DataProcessor>("LongTask", 1000, 5000);

    // Connect logger to the new processor - thread-safe connections
    logger.connectAllSignalsTo(longProcessor);

    // Start asynchronous execution
    auto future = stopDemoPool.run();

    // Wait briefly then request stop
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Requesting stop for all tasks..." << std::endl;
    stopDemoPool.stopAll();

    // Wait for completion
    future.wait();
    std::cout << "Stop demonstration completed" << std::endl;

    return 0;
}
```

## Licence
MIT

## Contact
fmaerten@gmail.com