[< Back](../index.md)

# Concurrent example
Example demonstrating how to use the ThreadPool and Runnable classes

```cpp
#include <iostream>
#include <random>
#include <sstream>
#include <task/Chronometer.h>
#include <task/Logger.h>
#include <task/concurrent/Runnable.h>
#include <task/concurrent/ThreadPool.h>
#include <thread>

// Example Runnable implementation: a worker that performs a calculation
class CalculationTask : public Runnable {
  public:
    CalculationTask(int id, int iterations)
        : m_id(id), m_iterations(iterations) {}

  protected:
    void runImpl() override {
        // Log the start of calculation
        std::stringstream ss;
        ss << "Task " << m_id << " starting calculation with " << m_iterations
           << " iterations";
        emitString("log", ss.str());

        // Random number generator for simulating varying workloads
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(50, 200);

        // Perform the calculation (simulated)
        double result = 0.0;
        for (int i = 0; i < m_iterations; ++i) {
            // Check if stop was requested
            if (stopRequested()) {
                emitString("warn", "Task " + std::to_string(m_id) +
                                       " stopped before completion");
                return;
            }

            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
            result += std::sin(static_cast<double>(i));

            // Report progress
            float progress = static_cast<float>(i + 1) / m_iterations;
            reportProgress(progress);
        }

        // Log the result
        ss.str("");
        ss << "Task " << m_id << " completed with result: " << result;
        emitString("log", ss.str());
    }

  private:
    int m_id;
    int m_iterations;
};

int main() {
    // Create a logger
    Logger logger("ThreadPool Example");

    // Create a chronometer to measure execution time
    Chronometer chrono;

    // Create a thread pool with verbose output
    ThreadPool pool(true);

    // Connect the logger to the pool
    pool.connectLoggerToAll(&logger);

    // Connect the chronometer to the pool
    chrono.connectData("timing", &logger, &Logger::log);
    pool.connectSimple("started", &chrono, &Chronometer::start);
    pool.connectSimple("finished", [&chrono]() {
        int64_t time = chrono.stop();
        std::cout << "Total execution time: " << time << " ms\n";
    });

    // Add tasks to the pool
    for (int i = 0; i < 5; ++i) {
        // Create tasks with different workloads
        pool.createAndAdd<CalculationTask>(i, 10 + i * 5);
    }

    // Execute all tasks in parallel
    std::cout << "Starting thread pool with " << pool.size() << " tasks...\n";

    try {
        // Run the pool
        pool.run();
    } catch (const std::exception &e) {
        std::cerr << "Error during execution: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All tasks completed successfully.\n";
    return 0;
}
```