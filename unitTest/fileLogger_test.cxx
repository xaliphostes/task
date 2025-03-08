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

#include "TEST.h"
#include <atomic>
#include <fstream>
#include <future>
#include <random>
#include <sstream>
#include <task/Algorithm.h>
#include <task/Chronometer.h>
#include <task/FileLogger.h>
#include <thread>

class TestAlgorithm : public Algorithm {
  public:
    void exec(const ArgumentPack &args = {}) override {
        for (int i = 0; i < 10; ++i) {
            if (stopRequested()) {
                emitString("warn", "Algorithm stopped");
                return;
            }

            // Log various messages
            emitString("log", "Processing step " + std::to_string(i));

            if (i % 3 == 0) {
                emitString("warn", "This step might need attention");
            }

            if (i == 8) {
                emitString("error", "Simulated error condition");
            }

            // Report progress
            reportProgress(static_cast<float>(i + 1) / 10.0f);

            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        emitString("log", "Algorithm completed");
    }
};

// Helper to check if a file contains a string
bool fileContains(const std::filesystem::path &filePath,
                  const std::string &text) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(text) != std::string::npos) {
            return true;
        }
    }

    return false;
}

// Helper to generate a temporary log directory
std::filesystem::path createTempLogDir() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(10000, 99999);

    std::filesystem::path tempDir =
        std::filesystem::temp_directory_path() /
        ("filelogger_test_" + std::to_string(distrib(gen)));

    std::filesystem::create_directories(tempDir);
    return tempDir;
}

// Test cases for FileLogger

TEST(FileLogger, BasicLogging) {
    // Create a temporary log directory
    auto tempDir = createTempLogDir();

    // Configure the logger
    FileLoggerConfig config;
    config.logDirectory = tempDir;
    config.filenamePattern = "test_log.txt";
    config.appendToFile = true;
    config.flushAfterEachWrite = true;

    // Create the logger
    FileLogger logger(config, LogLevel::Debug, "TestLogger");

    // Log some messages
    logger.logWithLevel(LogLevel::Info, "This is an info message");
    logger.logWithLevel(LogLevel::Warning, "This is a warning message");
    logger.logWithLevel(LogLevel::Error, "This is an error message");

    // Flush and get the log file path
    logger.flush();
    auto logPath = logger.getCurrentLogFilePath();

    // Verify file exists
    CHECK(std::filesystem::exists(logPath));

    // Verify content
    CHECK(fileContains(logPath, "INFO"));
    CHECK(fileContains(logPath, "WARNING"));
    CHECK(fileContains(logPath, "ERROR"));
    CHECK(fileContains(logPath, "This is an info message"));
    CHECK(fileContains(logPath, "This is a warning message"));
    CHECK(fileContains(logPath, "This is an error message"));

    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST(FileLogger, LogLevelFiltering) {
    // Create a temporary log directory
    auto tempDir = createTempLogDir();

    // Configure the logger
    FileLoggerConfig config;
    config.logDirectory = tempDir;
    config.filenamePattern = "filtered_log.txt";

    // Create the logger with Warning minimum level
    FileLogger logger(config, LogLevel::Warning, "FilterTest");

    // Log messages of different levels
    logger.logWithLevel(LogLevel::Debug,
                        "Debug message should be filtered out");
    logger.logWithLevel(LogLevel::Info, "Info message should be filtered out");
    logger.logWithLevel(LogLevel::Warning,
                        "Warning message should be included");
    logger.logWithLevel(LogLevel::Error, "Error message should be included");

    // Flush and get the log file path
    logger.flush();
    auto logPath = logger.getCurrentLogFilePath();

    // Verify content
    CHECK(!fileContains(logPath, "Debug message should be filtered out"));
    CHECK(!fileContains(logPath, "Info message should be filtered out"));
    CHECK(fileContains(logPath, "Warning message should be included"));
    CHECK(fileContains(logPath, "Error message should be included"));

    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST(FileLogger, FileRotation) {
    // Create a temporary log directory
    auto tempDir = createTempLogDir();

    // Configure the logger with small max file size to trigger rotation
    FileLoggerConfig config;
    config.logDirectory = tempDir;
    config.filenamePattern =
        "rotation_log_%N.txt"; // %N will be replaced with a number
    config.maxFileSize = 500;  // Small size to force rotation
    config.maxFiles = 3;

    // Create the logger
    FileLogger logger(config);

    // Variable to track rotation callback
    std::atomic<bool> rotationDetected(false);
    std::filesystem::path rotatedFilePath;

    // Register rotation callback
    logger.registerRotationCallback([&](const std::filesystem::path &path) {
        rotationDetected = true;
        rotatedFilePath = path;
    });

    // Write enough data to trigger rotation
    for (int i = 0; i < 20; i++) {
        std::string message = "This is log message " + std::to_string(i);
        message += " with some additional text to make it longer and trigger "
                   "rotation faster";
        logger.logWithLevel(LogLevel::Info, message);
    }

    // Verify rotation occurred
    CHECK(rotationDetected);
    CHECK(!rotatedFilePath.empty());
    CHECK(std::filesystem::exists(rotatedFilePath));

    // Get all log files
    int fileCount = 0;
    for (const auto &entry : std::filesystem::directory_iterator(tempDir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }

    // Should be at least 2 files (original + rotated)
    CHECK(fileCount >= 2);

    // Should not exceed maxFiles (3)
    CHECK(fileCount <= 3);

    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST(FileLogger, ConfigurationChanges) {
    // Create a temporary log directory
    auto tempDir = createTempLogDir();

    // Initial configuration
    FileLoggerConfig config1;
    config1.logDirectory = tempDir;
    config1.filenamePattern = "config_test_1.txt";
    config1.includeTimestamps = false;
    config1.includeLogLevel = true;

    // Create the logger
    FileLogger logger(config1);

    // Log a message with initial config
    logger.logWithLevel(LogLevel::Info, "Message with initial config");

    // Get the first log file path
    auto firstLogPath = logger.getCurrentLogFilePath();

    // New configuration
    FileLoggerConfig config2;
    config2.logDirectory = tempDir;
    config2.filenamePattern = "config_test_2.txt";
    config2.includeTimestamps = true;
    config2.includeLogLevel = false;

    // Update configuration
    logger.updateConfig(config2);

    // Log a message with new config
    logger.logWithLevel(LogLevel::Info, "Message with new config");

    // Get the second log file path
    auto secondLogPath = logger.getCurrentLogFilePath();

    // Verify files exist
    CHECK(std::filesystem::exists(firstLogPath));
    CHECK(std::filesystem::exists(secondLogPath));

    // Verify filenames changed
    CHECK(firstLogPath.filename() == "config_test_1.txt");
    CHECK(secondLogPath.filename() == "config_test_2.txt");

    // Verify content format changes
    CHECK(
        fileContains(firstLogPath, "INFO")); // First file should have log level
    CHECK(!fileContains(secondLogPath,
                        "INFO")); // Second file should not have log level

    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST(FileLogger, IntegrationWithTasks) {
    // Create a temporary log directory
    auto tempDir = createTempLogDir();

    // Configure the logger
    FileLoggerConfig config;
    config.logDirectory = tempDir;
    config.filenamePattern = "algorithm_log.txt";

    // Create the logger and algorithm
    auto logger = std::make_shared<FileLogger>(config);
    auto algorithm = std::make_shared<TestAlgorithm>();
    auto chrono = std::make_shared<Chronometer>();

    // Connect logger to algorithm and chronometer
    logger->connectAllSignalsTo(algorithm.get());
    logger->connectAllSignalsTo(chrono.get());

    // Execute the algorithm with timing
    chrono->start();
    algorithm->exec();
    auto elapsed = chrono->stop();

    // Log execution time
    ArgumentPack timeArgs;
    timeArgs.add<std::string>("Algorithm completed in " +
                              std::to_string(elapsed) + " ms");
    logger->log(timeArgs);

    // Flush log and get path
    logger->flush();
    auto logPath = logger->getCurrentLogFilePath();

    // Verify file exists
    CHECK(std::filesystem::exists(logPath));

    // Verify algorithm logs were captured
    CHECK(fileContains(logPath, "Processing step"));
    CHECK(fileContains(logPath, "This step might need attention"));
    CHECK(fileContains(logPath, "Simulated error condition"));
    CHECK(fileContains(logPath, "Algorithm completed"));

    // Verify chrono log was captured
    CHECK(fileContains(logPath, "Algorithm completed in"));

    // Clean up
    std::filesystem::remove_all(tempDir);
}

TEST(FileLogger, MultiThreadedAccess) {
    // Create a temporary log directory
    auto tempDir = createTempLogDir();

    // Configure the logger
    FileLoggerConfig config;
    config.logDirectory = tempDir;
    config.filenamePattern = "threaded_log.txt";

    // Create the logger
    auto logger = std::make_shared<FileLogger>(config);

    // Number of threads and messages per thread
    const int numThreads = 10;
    const int messagesPerThread = 50;

    // Use a vector to store futures
    std::vector<std::future<void>> futures;

    // Launch threads
    for (int t = 0; t < numThreads; t++) {
        futures.push_back(std::async(std::launch::async, [t, messagesPerThread,
                                                          logger]() {
            for (int i = 0; i < messagesPerThread; i++) {
                std::string message = "Thread " + std::to_string(t) +
                                      " message " + std::to_string(i);
                logger->logWithLevel(LogLevel::Info, message);

                // Occasionally log at other levels
                if (i % 10 == 0) {
                    logger->logWithLevel(LogLevel::Warning,
                                         "Thread " + std::to_string(t) +
                                             " warning " + std::to_string(i));
                }

                if (i % 20 == 0) {
                    logger->logWithLevel(LogLevel::Error,
                                         "Thread " + std::to_string(t) +
                                             " error " + std::to_string(i));
                }

                // Small sleep to simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }));
    }

    // Wait for all threads to complete
    for (auto &future : futures) {
        future.wait();
    }

    // Flush log and get path
    logger->flush();
    auto logPath = logger->getCurrentLogFilePath();

    // Verify file exists
    CHECK(std::filesystem::exists(logPath));

    // Count log entries (there should be numThreads * messagesPerThread + some
    // extra for warnings and errors)
    std::ifstream logFile(logPath);
    int lineCount = 0;
    std::string line;
    while (std::getline(logFile, line)) {
        lineCount++;
    }

    // The total should be at least the base number of messages
    // Each thread writes messagesPerThread regular messages + warnings + errors
    int expectedMinimum = numThreads * messagesPerThread;
    CHECK(lineCount >= expectedMinimum);

    // Clean up
    std::filesystem::remove_all(tempDir);
}

// Run all the tests
RUN_TESTS()