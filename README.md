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

# Main Example: Task Library Demo
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

# Custom Algorithm Example
The second example shows how to implement custom algorithms using the library:
- **DataAnalyzer**: Extends Algorithm to analyze numerical data and compute statistics
- **BatchAnalyzer**: Extends FlowAlgorithm to process multiple datasets in sequence

This example demonstrates:
- Creating **custom algorithm** implementations
- **Signal emission with custom data** types
- **Flow-based** processing with jobs
- **Error handling** with try-catch blocks
- **Progress** tracking in multi-stage operations

Key Features Demonstrated
- **Signal-Slot** System: Components communicate through signals and slots
- **Progress Reporting**: Tasks report progress that can be monitored
- **Parallel Execution**: Tasks run concurrently in thread pools
- **Cancellation Support**: Tasks can check for stop requests
- **Logging Integration**: All components can emit log messages
- **Timing Measurements**: Chronometer tracks execution time
- **Flow-Based Processing**: Sequential processing of multiple jobs

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

// Include necessary headers for the task library
#include <task/Algorithm.h>
#include <task/FlowAlgorithm.h>
#include <task/Logger.h>

// A custom algorithm that processes numerical data
class DataAnalyzer : public Algorithm {
public:
    DataAnalyzer() {
        createDataSignal("result");
        createDataSignal("stats");
    }
    
    void setData(const std::vector<double>& data) {
        m_data = data;
        setDirty(true);
    }
    
    void exec(const ArgumentPack& args = {}) override {
        if (m_data.empty()) {
            emitString("error", "No data to analyze");
            return;
        }
        
        size_t size = m_data.size();
        emitString("log", "Analyzing " + std::to_string(size) + " data points");
        
        // Calculate some statistics
        double sum = 0;
        double min = m_data[0];
        double max = m_data[0];
        
        for (size_t i = 0; i < size && !stopRequested(); ++i) {
            // Simulate intensive work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            double value = m_data[i];
            sum += value;
            min = std::min(min, value);
            max = std::max(max, value);
            
            // Report progress
            float progress = static_cast<float>(i + 1) / static_cast<float>(size);
            reportProgress(progress);
            
            // Occasional log message
            if ((i + 1) % (size / 10) == 0 || i + 1 == size) {
                emitString("log", "Processed " + std::to_string(i + 1) + " / " + 
                          std::to_string(size) + " data points");
            }
        }
        
        if (stopRequested()) {
            emitString("warn", "Analysis was stopped early");
            return;
        }
        
        // Calculate final statistics
        double mean = sum / size;
        
        // Calculate standard deviation
        double variance = 0;
        for (const auto& value : m_data) {
            variance += (value - mean) * (value - mean);
        }
        double stdDev = std::sqrt(variance / size);
        
        // Emit results
        ArgumentPack resultArgs;
        resultArgs.add<double>(mean);
        resultArgs.add<double>(stdDev);
        emit("result", resultArgs);
        
        // Emit detailed stats
        ArgumentPack statsArgs;
        statsArgs.add<double>(min);
        statsArgs.add<double>(max);
        statsArgs.add<double>(mean);
        statsArgs.add<double>(stdDev);
        statsArgs.add<size_t>(size);
        emit("stats", statsArgs);
        
        emitString("log", "Analysis completed: Mean = " + std::to_string(mean) + 
                  ", StdDev = " + std::to_string(stdDev));
    }
    
private:
    std::vector<double> m_data;
};

// A flow algorithm that processes a collection of datasets
class BatchAnalyzer : public FlowAlgorithm {
public:
    BatchAnalyzer() {
        createDataSignal("batchResults");
    }
    
    void exec(const ArgumentPack& args = {}) override {
        emitString("log", "Starting batch analysis with " + 
                  std::to_string(m_jobs.size()) + " datasets");
        
        // Create a collection to store results
        std::vector<double> means;
        std::vector<double> stdDevs;
        
        // Process each job
        for (size_t i = 0; i < m_jobs.size() && !stopRequested(); ++i) {
            // Report progress
            float progress = static_cast<float>(i) / static_cast<float>(m_jobs.size());
            reportProgress(progress);
            
            try {
                // Process this dataset
                emitString("log", "Processing dataset " + std::to_string(i + 1));
                doJob(m_jobs[i]);
            }
            catch (const std::exception& e) {
                emitString("error", "Error processing dataset " + 
                          std::to_string(i + 1) + ": " + e.what());
            }
        }
        
        if (stopRequested()) {
            emitString("warn", "Batch analysis was stopped early");
        } else {
            emitString("log", "Batch analysis completed");
        }
    }
    
    void doJob(const std::any& job) override {
        try {
            // Try to cast to vector<double>
            const auto& dataset = std::any_cast<const std::vector<double>&>(job);
            
            // Create analyzer for this dataset
            DataAnalyzer analyzer;
            analyzer.setData(dataset);
            
            // Connect signals
            analyzer.connectData("log", [this](const ArgumentPack& args) {
                this->emit("log", args);
            });
            
            // Collect results
            std::optional<double> mean;
            std::optional<double> stdDev;
            
            analyzer.connectData("result", [&mean, &stdDev](const ArgumentPack& args) {
                mean = args.get<double>(0);
                stdDev = args.get<double>(1);
            });
            
            // Execute analysis
            analyzer.exec();
            
            // Check for results
            if (mean && stdDev) {
                emitString("log", "Dataset result: Mean = " + std::to_string(*mean) + 
                          ", StdDev = " + std::to_string(*stdDev));
            } else {
                emitString("warn", "Analysis did not produce results");
            }
        }
        catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid dataset format");
        }
    }
};

// Example usage of the custom algorithms
void runCustomAlgorithmExample() {
    // Create a logger
    Logger logger("CustomAlgo");
    
    // Create random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> distrib(100.0, 15.0);
    
    std::vector<double> dataset1;
    for (int i = 0; i < 1000; ++i) {
        dataset1.push_back(distrib(gen));
    }
    
    // Single dataset analysis
    std::cout << "\n=== Single Dataset Analysis ===" << std::endl;
    DataAnalyzer analyzer;
    analyzer.setData(dataset1);
    
    // Connect analyzer to logger
    logger.connectAllSignalsTo(&analyzer);
    
    // Connect to handle results
    analyzer.connectData("result", [](const ArgumentPack& args) {
        double mean = args.get<double>(0);
        double stdDev = args.get<double>(1);
        std::cout << "Analysis Result: Mean = " << mean 
                  << ", Standard Deviation = " << stdDev << std::endl;
    });
    
    // Run the analysis
    analyzer.run();
    
    // Batch analysis with multiple datasets
    std::cout << "\n=== Batch Analysis ===" << std::endl;
    
    // Create batch analyzer
    BatchAnalyzer batchAnalyzer;
    
    // Connect to logger
    logger.connectAllSignalsTo(&batchAnalyzer);
    
    // Create multiple datasets with different distributions
    for (int i = 0; i < 5; ++i) {
        std::vector<double> dataset;
        std::normal_distribution<> dist(50.0 + i * 20.0, 5.0 + i * 2.0);
        
        for (int j = 0; j < 500; ++j) {
            dataset.push_back(dist(gen));
        }
        
        // Add to batch analyzer
        batchAnalyzer.addJob(dataset);
    }
    
    // Run batch analysis
    batchAnalyzer.run();
}
```
## Licence
MIT

## Contact
fmaerten@gmail.com
