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

Minimalist Reactive library in C++

## Requirements
- C++20 (but C++23 soon)
- cmake

# Compilation
Create a `build` directory, **go inside** and type
```sh
cmake .. && make -j12
```

# Running unit tests
**NOTE**: The internal cmake test is used to perform unit testing.

In the **same directory** as for the compilation (i.e., the `build` directory), only type
```sh
ctest
```
or
```sh
make test
```

# [Documentation in markdown](docs/index.md)

# [Documentation in html](docs/html/index.html)


# Custom Algorithm Example
This example demonstrates a comprehensive sensor data processing pipeline that leverages all the new features of this library.

The example creates a robust pipeline for processing IoT sensor data that showcases the power of the enhanced task library

It also demonstrates important operational features like state persistence, performance profiling, and graceful shutdown handling.

This example showcases how the enhanced task library could be used to build sophisticated, resilient data processing systems with minimal boilerplate code, allowing developers to focus on the business logic rather than the infrastructure.

See this [**README**](examples/complete-example/README.md) in the `example/complete-example` folder


# Task Library Demo
The first example showcases a parallel data processing system with the following components:
- **DataProcessor**: A custom Runnable class that simulates processing data with progress reporting
- **ProgressMonitor**: A custom Task class that tracks and responds to progress events
- **ThreadPool**: Used to manage and execute multiple tasks in parallel
- **Logger**: Connected to all components to capture and display log messages
- **Chronometer**: Used to measure execution time

The example demonstrates:
- **Signal-slot** connections between components
- **Progress** reporting and monitoring
- **Asynchronous task** execution
- **Thread pool** management
- **Task cancellation** with stopRequested()

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

// Include necessary headers for the task library
#include <task/Task.h>
#include <task/Logger.h>
#include <task/ProgressMonitor.h>
#include <task/concurrent/ThreadPool.h>
#include <task/concurrent/Runnable.h>
#include <task/Chronometer.h>

// Define a custom data processor task
class DataProcessor : public Runnable {
public:
    DataProcessor(const std::string& name, int dataSize, int processingTime) 
        : m_name(name), m_dataSize(dataSize), m_processingTime(processingTime) {}

protected:
    void runImpl() override {
        emitString("log", m_name + " started processing " + std::to_string(m_dataSize) + " data points");
        
        // Simulate processing with progress updates
        for (int i = 0; i < 10 && !stopRequested(); ++i) {
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(m_processingTime / 10));
            
            // Report progress (0.0 to 1.0)
            float progress = static_cast<float>(i + 1) / 10.0f;
            reportProgress(progress);
            
            // Log progress milestone
            if ((i + 1) % 3 == 0) {
                emitString("log", m_name + " completed " + std::to_string((i + 1) * 10) + "% of processing");
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
    // Create a logger
    auto logger = std::make_unique<Logger>("App");
    
    // Create a chronometer to measure execution time
    auto chrono = std::make_unique<Chronometer>();
    
    // Create a progress monitor
    auto progressMonitor = std::make_unique<ProgressMonitor>();
    
    // Connect chronometer to logger
    logger->connectAllSignalsTo(chrono.get());
    
    // Create a thread pool
    auto threadPool = std::make_unique<ThreadPool>();
    
    // Connect logger to thread pool
    logger->connectAllSignalsTo(threadPool.get());
    
    // Create a random number generator for data sizes and processing times
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dataSizeDist(100, 1000);
    std::uniform_int_distribution<> processingTimeDist(200, 800);
    
    // Create and add processing tasks to the thread pool
    int numTasks = 5;
    progressMonitor->setTaskCount(numTasks);
    
    for (int i = 0; i < numTasks; i++) {
        std::string taskName = "Processor-" + std::to_string(i + 1);
        int dataSize = dataSizeDist(gen);
        int processingTime = processingTimeDist(gen);
        
        auto processor = threadPool->createAndAdd<DataProcessor>(taskName, dataSize, processingTime);
        
        // Connect signals between processor and monitor
        processor->connectSimple("started", progressMonitor.get(), &ProgressMonitor::onTaskStarted);
        processor->connectSimple("finished", progressMonitor.get(), &ProgressMonitor::onTaskFinished);
        processor->connectData("progress", progressMonitor.get(), &ProgressMonitor::onProgress);
        
        // Connect logger to processor
        logger->connectAllSignalsTo(processor);
    }
    
    // Connect summary signal from monitor to a lambda that prints final stats
    progressMonitor->connectData("summary", [](const ArgumentPack& args) {
        int totalTasks = args.get<int>(0);
        int completedTasks = args.get<int>(1);
        std::cout << "\n===== Summary =====\n";
        std::cout << "Total tasks: " << totalTasks << "\n";
        std::cout << "Completed tasks: " << completedTasks << "\n";
        std::cout << "===================" << std::endl;
    });
    
    // Start timing
    std::cout << "Starting parallel processing with " << numTasks << " tasks..." << std::endl;
    chrono->start();
    
    // Execute all tasks in the thread pool
    threadPool->exec();
    
    // Stop timing and display results
    int64_t elapsed = chrono->stop();
    std::cout << "All tasks completed in " << elapsed << " ms" << std::endl;
    
    // Handle a stop request demonstration (for a new set of tasks)
    std::cout << "\nDemonstrating stop functionality:" << std::endl;
    
    // Create a new processor and add it to a new thread pool
    auto stopDemoPool = std::make_unique<ThreadPool>();
    auto longProcessor = stopDemoPool->createAndAdd<DataProcessor>("LongTask", 1000, 5000);
    
    // Connect logger to the new processor
    logger->connectAllSignalsTo(longProcessor);
    
    // Start asynchronous execution
    auto future = stopDemoPool->run();
    
    // Wait briefly then request stop
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Requesting stop for all tasks..." << std::endl;
    stopDemoPool->stopAll();
    
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
