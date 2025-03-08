/*
 * Copyright (c) 2025-now fmaerten@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <task/Chronometer.h>
#include <task/FileLogger.h>
#include <task/concurrent/Runnable.h>
#include <task/concurrent/ThreadPool.h>
#include <thread>

namespace fs = std::filesystem;

/**
 * Dummy worker task that generates various log messages
 */
class WorkerTask : public Runnable {
  public:
    WorkerTask(const std::string &name, int workItems)
        : m_name(name), m_workItems(workItems) {}

  protected:
    void runImpl() override {
        emitString("log", m_name + " started processing " +
                              std::to_string(m_workItems) + " items");

        // Random number generator for simulating issues
        std::random_device rd;
        std::mt19937 gen(rd());

        // 1 in 10 chance of warning
        std::uniform_int_distribution<> warningDist(1, 10);
        // 1 in 30 chance of error
        std::uniform_int_distribution<> errorDist(1, 30);

        for (int i = 0; i < m_workItems; ++i) {
            // Check for cancellation
            if (stopRequested()) {
                emitString("warn", m_name + " was stopped before completion");
                return;
            }

            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // Occasionally emit warnings or errors
            if (warningDist(gen) == 1) {
                emitString("warn", m_name +
                                       " encountered a minor issue at item " +
                                       std::to_string(i));
            }

            if (errorDist(gen) == 1) {
                emitString("error",
                           m_name + " encountered an error processing item " +
                               std::to_string(i));
            }

            // Report progress
            float progress = float(i + 1) / float(m_workItems);
            reportProgress(progress);

            // Log milestone progress
            if ((i + 1) % 5 == 0 || i + 1 == m_workItems) {
                emitString("log", m_name + " processed " +
                                      std::to_string(i + 1) + "/" +
                                      std::to_string(m_workItems) + " items");
            }
        }

        emitString("log", m_name + " completed successfully");
    }

  private:
    std::string m_name;
    int m_workItems;
};

int main() {
    // Create logs directory if it doesn't exist
    fs::path logsDir = "example_logs";
    fs::create_directories(logsDir);

    std::cout << "FileLogger Example Application" << std::endl;
    std::cout << "-----------------------------" << std::endl;

    // Configure the file logger
    FileLoggerConfig config{
        .logDirectory = logsDir,
        .filenamePattern = "app_%Y%m%d_%H%M%S.log",
        .maxFileSize = 1024 * 1024, // 1MB per file
        .maxFiles = 5,              // Keep only 5 most recent log files
        .includeTaskName = true,    // Include originating task name in logs
    };

    // Create the file logger
    FileLogger logger(config, LogLevel::Debug, "MainApp");

    // Create a chronometer for timing
    Chronometer chrono;

    // Connect chronometer to logger
    logger.connectAllSignalsTo(&chrono);

    // Log application start
    logger.logWithLevel(LogLevel::Info, "Application started");

    // Set up file rotation notification
    logger.registerRotationCallback([&logger](const fs::path &oldPath) {
        logger.logWithLevel(LogLevel::Info, "Log file rotated. Previous log: " +
                                                oldPath.filename().string());
    });

    // Create a thread pool for processing work
    ThreadPool threadPool;

    // Connect thread pool to logger
    logger.connectAllSignalsTo(&threadPool);

    // Log thread pool information
    logger.logWithLevel(LogLevel::Info,
                        "Created thread pool with " +
                            std::to_string(ThreadPool::maxThreadCount()) +
                            " hardware threads");

    // Number of worker tasks to create
    const int numWorkers = 4;

    // Create and add workers to the thread pool
    for (int i = 0; i < numWorkers; i++) {
        std::string workerName = "Worker-" + std::to_string(i + 1);
        int workItems = 20 + i * 5; // Different amount of work for each worker

        WorkerTask worker(workerName, workItems);

        // Connect worker to logger
        logger.connectAllSignalsTo(&worker);

        logger.logWithLevel(LogLevel::Info, "Created " + workerName + " with " +
                                                std::to_string(workItems) +
                                                " work items");
    }

    // Start timing
    chrono.start();

    // Log execution start
    logger.logWithLevel(LogLevel::Info,
                        "Starting parallel execution of workers");

    // Execute all workers
    threadPool.exec();

    // Stop timing
    int64_t elapsedMs = chrono.stop();

    // Log execution completion
    logger.logWithLevel(LogLevel::Info, "All workers completed in " +
                                            std::to_string(elapsedMs) + " ms");

    // Log some application statistics
    logger.logWithLevel(LogLevel::Info, "Processed a total of " +
                                            std::to_string(threadPool.size()) +
                                            " worker tasks");

    // Demonstrate different log levels
    logger.logWithLevel(LogLevel::Debug,
                        "This is a debug message (detailed information)");
    logger.logWithLevel(LogLevel::Info,
                        "This is an info message (general information)");
    logger.logWithLevel(LogLevel::Warning,
                        "This is a warning message (potential issue)");
    logger.logWithLevel(LogLevel::Error,
                        "This is an error message (operation failed)");
    logger.logWithLevel(LogLevel::Fatal,
                        "This is a fatal message (system failure)");

    // Log application end
    logger.logWithLevel(LogLevel::Info, "Application completed successfully");

    // Final output
    std::cout << "Processing completed in " << elapsedMs << " ms" << std::endl;
    std::cout << "Logs have been written to: "
              << std::filesystem::absolute(logsDir) << std::endl;

    return 0;
}