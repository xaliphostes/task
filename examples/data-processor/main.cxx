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