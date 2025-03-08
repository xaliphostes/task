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

#pragma once
#include "Logger.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Configuration options for file-based logging
 */
struct FileLoggerConfig {
    // -----------------------
    // Filename and path settings
    // -----------------------

    // Directory for log files
    std::filesystem::path logDirectory = "logs";

    // Filename with optional time format patterns
    std::string filenamePattern = "log_%Y%m%d.txt";

    // Auto-create log directory
    bool createDirectoryIfNotExists = true;

    // -----------------------
    // File behavior settings
    // -----------------------

    // Whether to append to existing log files
    bool appendToFile = true;

    // Force flush after each log entry
    bool flushAfterEachWrite = true;

    // Max size in bytes (10 MB default), 0 = unlimited
    std::streamsize maxFileSize = 10 * 1024 * 1024;

    // Maximum number of log files to keep (0 = unlimited)
    int maxFiles = 5;

    // -----------------------
    // Timestamp settings
    // -----------------------

    // Timestamp format for log entries
    std::string timestampFormat = "[%Y-%m-%d %H:%M:%S] ";

    // Whether to add timestamps to log entries
    bool includeTimestamps = true;

    // -----------------------
    // Log format settings
    // -----------------------

    // Whether to include log level in output
    bool includeLogLevel = true;

    // Separator between timestamp, level, and message
    std::string logSeparator = " | ";

    // Whether to include originating task name (if available)
    bool includeTaskName = false;
};

/**
 * @brief Log level enumeration for categorizing log messages
 */
enum class LogLevel {
    Debug,   // Detailed information for debugging
    Info,    // General information about system operation
    Warning, // Potential issues that aren't critical
    Error,   // Error conditions that may affect operation
    Fatal    // Severe errors that may cause the program to terminate
};

/**
 * @brief A file-based logger that extends the Task Logger with filesystem
 * capabilities
 *
 * The FileLogger writes log messages to files with configurable naming,
 * rotation, and formatting options. It's thread-safe and integrates with the
 * Task framework's signal-slot system.
 */
class FileLogger : public Logger {
  public:
    /**
     * @brief Constructor with optional configuration and log level
     * @param config The configuration settings for the file logger
     * @param minimumLevel The minimum log level to record (default: Debug)
     * @param taskName Optional name to identify this logger
     */
    explicit FileLogger(const FileLoggerConfig &config = FileLoggerConfig(),
                        LogLevel minimumLevel = LogLevel::Debug,
                        const std::string &taskName = "FileLogger");

    /**
     * @brief Destructor that ensures log file is properly closed
     */
    virtual ~FileLogger();

    /**
     * @brief Override to log messages to file in addition to base behavior
     * @param args The ArgumentPack containing the log message
     */
    void log(const ArgumentPack &args) override;

    /**
     * @brief Override to log warnings to file in addition to base behavior
     * @param args The ArgumentPack containing the warning message
     */
    void warn(const ArgumentPack &args) override;

    /**
     * @brief Override to log errors to file in addition to base behavior
     * @param args The ArgumentPack containing the error message
     */
    void error(const ArgumentPack &args) override;

    /**
     * @brief Explicitly log a message with a specific log level
     * @param level The severity level of the message
     * @param message The message to log
     * @param taskName Optional name of the originating task
     */
    void logWithLevel(LogLevel level, const std::string &message,
                      const std::string &taskName = "");

    /**
     * @brief Set the minimum log level that will be recorded
     * @param level The new minimum log level
     */
    void setMinimumLogLevel(LogLevel level);

    /**
     * @brief Get the current minimum log level
     * @return The current minimum log level
     */
    LogLevel getMinimumLogLevel() const;

    /**
     * @brief Update the logger configuration
     * @param config The new configuration settings
     * @return true if successful, false if there was an error
     */
    bool updateConfig(const FileLoggerConfig &config);

    /**
     * @brief Get the current logger configuration
     * @return A copy of the current configuration
     */
    FileLoggerConfig getConfig() const;

    /**
     * @brief Get the path of the current log file
     * @return The full path to the current log file or empty if not logging to
     * file
     */
    std::filesystem::path getCurrentLogFilePath() const;

    /**
     * @brief Rotate the log file, closing the current one and creating a new
     * one This can be called manually, but also happens automatically when file
     * size limits are reached
     * @return true if rotation was successful, false otherwise
     */
    bool rotateLogFile();

    /**
     * @brief Flush any buffered log data to disk
     */
    void flush();

    /**
     * @brief Register a callback to be notified when log rotation occurs
     * @param callback Function to call when a log file is rotated
     * @return An ID that can be used to unregister the callback
     */
    int registerRotationCallback(
        std::function<void(const std::filesystem::path &)> callback);

    /**
     * @brief Unregister a previously registered rotation callback
     * @param callbackId The ID returned from registerRotationCallback
     * @return true if the callback was found and removed, false otherwise
     */
    bool unregisterRotationCallback(int callbackId);

  private:
    FileLoggerConfig m_config;
    LogLevel m_minimumLevel;
    std::optional<std::ofstream> m_logFile;
    std::filesystem::path m_currentLogFilePath;
    mutable std::mutex m_logMutex;
    std::streamsize m_currentFileSize;
    std::string m_taskName;
    int m_nextCallbackId;
    std::map<int, std::function<void(const std::filesystem::path &)>>
        m_rotationCallbacks;

    /**
     * @brief Convert a LogLevel to its string representation
     * @param level The log level to convert
     * @return String representation of the log level
     */
    std::string logLevelToString(LogLevel level) const;

    /**
     * @brief Format a timestamp according to the configured format
     * @param time The time point to format (default: current time)
     * @return Formatted timestamp string
     */
    std::string formatTimestamp(std::chrono::system_clock::time_point time =
                                    std::chrono::system_clock::now()) const;

    /**
     * @brief Format a filename according to the configured pattern
     * @param time The time point to use for time-based patterns (default:
     * current time)
     * @return Formatted filename
     */
    std::string formatFilename(std::chrono::system_clock::time_point time =
                                   std::chrono::system_clock::now()) const;

    /**
     * @brief Initialize the log file, creating directories and opening the file
     * @return true if successful, false otherwise
     */
    bool initializeLogFile();

    /**
     * @brief Write a message to the log file
     * @param level The severity level of the message
     * @param message The message to write
     * @param taskName Optional name of the originating task
     * @return true if successful, false otherwise
     */
    bool writeToFile(LogLevel level, const std::string &message,
                     const std::string &taskName = "");

    /**
     * @brief Clean up old log files if maximum file count is exceeded
     */
    void cleanupOldLogFiles();

    /**
     * @brief Get all log files in the log directory
     * @return Vector of paths to log files, sorted by modification time (newest
     * first)
     */
    std::vector<std::filesystem::path> getLogFiles() const;

    /**
     * @brief Notify rotation callbacks about a log file rotation
     * @param oldFilePath Path to the file that was just rotated out
     */
    void notifyRotationCallbacks(const std::filesystem::path &oldFilePath);
};