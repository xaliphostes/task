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

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <task/FileLogger.h>

FileLogger::FileLogger(const FileLoggerConfig &config, LogLevel minimumLevel,
                       const std::string &taskName)
    : Logger(taskName), m_config(config), m_minimumLevel(minimumLevel),
      m_currentFileSize(0), m_taskName(taskName), m_nextCallbackId(0) {

    // Try to initialize the log file
    if (!initializeLogFile()) {
        // Log initialization failure to standard error
        std::cerr << "Failed to initialize log file. Logging to file will be "
                     "disabled."
                  << std::endl;
    }

    // Create additional signals for file-specific operations
    createSignal("fileRotated");
    createSignal("fileError");
}

FileLogger::~FileLogger() {
    // Ensure the log file is properly closed
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (m_logFile && m_logFile->is_open()) {
        m_logFile->flush();
        m_logFile->close();
    }
}

void FileLogger::log(const ArgumentPack &args) {
    // Call the base class implementation for console logging
    Logger::log(args);

    // Extract message if possible
    if (!args.empty()) {
        try {
            std::string message = args.get<std::string>(0);
            writeToFile(LogLevel::Info, message);
        } catch (const std::exception &) {
            // Ignore bad cast errors
        }
    }
}

void FileLogger::warn(const ArgumentPack &args) {
    // Call the base class implementation for console logging
    Logger::warn(args);

    // Extract message if possible
    if (!args.empty()) {
        try {
            std::string message = args.get<std::string>(0);
            writeToFile(LogLevel::Warning, message);
        } catch (const std::exception &) {
            // Ignore bad cast errors
        }
    }
}

void FileLogger::error(const ArgumentPack &args) {
    // Call the base class implementation for console logging
    Logger::error(args);

    // Extract message if possible
    if (!args.empty()) {
        try {
            std::string message = args.get<std::string>(0);
            writeToFile(LogLevel::Error, message);
        } catch (const std::exception &) {
            // Ignore bad cast errors
        }
    }
}

void FileLogger::logWithLevel(LogLevel level, const std::string &message,
                              const std::string &taskName) {
    // Skip if below minimum level
    if (level < m_minimumLevel) {
        return;
    }

    // Log to file
    writeToFile(level, message, taskName);

    // Also log to console based on level
    ArgumentPack args;
    args.add<std::string>(message);

    switch (level) {
    case LogLevel::Debug:
    case LogLevel::Info:
        Logger::log(args);
        break;
    case LogLevel::Warning:
        Logger::warn(args);
        break;
    case LogLevel::Error:
    case LogLevel::Fatal:
        Logger::error(args);
        break;
    }
}

void FileLogger::setMinimumLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_minimumLevel = level;
}

LogLevel FileLogger::getMinimumLogLevel() const {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_minimumLevel;
}

bool FileLogger::updateConfig(const FileLoggerConfig &config) {
    std::lock_guard<std::mutex> lock(m_logMutex);

    // Store the old configuration
    FileLoggerConfig oldConfig = m_config;

    // Update with new configuration
    m_config = config;

    // Check if we need to reinitialize the log file
    if (oldConfig.logDirectory != config.logDirectory ||
        oldConfig.filenamePattern != config.filenamePattern ||
        oldConfig.appendToFile != config.appendToFile) {

        // Close the current log file if open
        if (m_logFile && m_logFile->is_open()) {
            m_logFile->close();
            m_logFile.reset();
        }

        // Reinitialize with new settings
        return initializeLogFile();
    }

    return true;
}

FileLoggerConfig FileLogger::getConfig() const {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_config;
}

std::filesystem::path FileLogger::getCurrentLogFilePath() const {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_currentLogFilePath;
}

bool FileLogger::rotateLogFile() {
    std::lock_guard<std::mutex> lock(m_logMutex);

    // Store the old file path for callbacks
    auto oldFilePath = m_currentLogFilePath;

    // Close current log file if open
    if (m_logFile && m_logFile->is_open()) {
        m_logFile->close();
        m_logFile.reset();
    }

    // Initialize a new log file
    bool success = initializeLogFile();

    if (success) {
        // Notify callbacks about rotation
        notifyRotationCallbacks(oldFilePath);

        // Emit signal for rotation
        emit("fileRotated");

        // Clean up old log files if needed
        cleanupOldLogFiles();
    } else {
        // Emit error signal
        ArgumentPack args;
        args.add<std::string>("Failed to rotate log file");
        emit("fileError", args);
    }

    return success;
}

void FileLogger::flush() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (m_logFile && m_logFile->is_open()) {
        m_logFile->flush();
    }
}

int FileLogger::registerRotationCallback(
    std::function<void(const std::filesystem::path &)> callback) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    int id = m_nextCallbackId++;
    m_rotationCallbacks[id] = callback;
    return id;
}

bool FileLogger::unregisterRotationCallback(int callbackId) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_rotationCallbacks.erase(callbackId) > 0;
}

std::string FileLogger::logLevelToString(LogLevel level) const {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Fatal:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

std::string
FileLogger::formatTimestamp(std::chrono::system_clock::time_point time) const {
    std::time_t timeT = std::chrono::system_clock::to_time_t(time);
    std::tm tm;

    // Use thread-safe version of localtime
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, m_config.timestampFormat.c_str());
    return oss.str();
}

std::string
FileLogger::formatFilename(std::chrono::system_clock::time_point time) const {
    std::time_t timeT = std::chrono::system_clock::to_time_t(time);
    std::tm tm;

    // Use thread-safe version of localtime
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, m_config.filenamePattern.c_str());
    return oss.str();
}

bool FileLogger::initializeLogFile() {
    try {
        // Create log directory if it doesn't exist and is configured to create
        if (m_config.createDirectoryIfNotExists) {
            std::filesystem::create_directories(m_config.logDirectory);
        } else if (!std::filesystem::exists(m_config.logDirectory)) {
            std::cerr << "Log directory does not exist: "
                      << m_config.logDirectory << std::endl;
            return false;
        }

        // Format the filename with current time
        std::string filename = formatFilename();

        // Construct the full path
        m_currentLogFilePath = m_config.logDirectory / filename;

        // Open the log file with appropriate flags
        std::ios_base::openmode mode = std::ios::out;
        if (m_config.appendToFile) {
            mode |= std::ios::app;
        }

        m_logFile.emplace(m_currentLogFilePath, mode);

        if (!m_logFile->is_open()) {
            std::cerr << "Failed to open log file: " << m_currentLogFilePath
                      << std::endl;
            m_logFile.reset();
            return false;
        }

        // Get the current file size
        if (std::filesystem::exists(m_currentLogFilePath)) {
            m_currentFileSize =
                std::filesystem::file_size(m_currentLogFilePath);
        } else {
            m_currentFileSize = 0;
        }

        // Write an initialization message
        *m_logFile << "\n";
        if (m_config.includeTimestamps) {
            *m_logFile << formatTimestamp();
        }

        if (m_config.includeLogLevel) {
            *m_logFile << "INFO" << m_config.logSeparator;
        }

        *m_logFile << "Log file initialized";

        if (m_config.includeTaskName && !m_taskName.empty()) {
            *m_logFile << " (" << m_taskName << ")";
        }

        *m_logFile << std::endl;

        // Update file size
        m_currentFileSize += static_cast<std::streamsize>(m_logFile->tellp());

        if (m_config.flushAfterEachWrite) {
            m_logFile->flush();
        }

        return true;

    } catch (const std::exception &e) {
        std::cerr << "Error initializing log file: " << e.what() << std::endl;
        m_logFile.reset();
        return false;
    }
}

bool FileLogger::writeToFile(LogLevel level, const std::string &message,
                             const std::string &taskName) {
    // Skip if below minimum level
    if (level < m_minimumLevel) {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_logMutex);

    try {
        // Make sure we have an open log file
        if (!m_logFile || !m_logFile->is_open()) {
            if (!initializeLogFile()) {
                return false;
            }
        }

        // Check if we need to rotate the file based on size
        if (m_config.maxFileSize > 0 &&
            m_currentFileSize >= m_config.maxFileSize) {
            if (!rotateLogFile()) {
                return false;
            }
        }

        // Calculate approximate size of the log entry
        std::streamsize entrySize =
            static_cast<std::streamsize>(message.size());

        // Add timestamp if configured
        if (m_config.includeTimestamps) {
            std::string timestamp = formatTimestamp();
            *m_logFile << timestamp;
            entrySize += static_cast<std::streamsize>(timestamp.size());
        }

        // Add log level if configured
        if (m_config.includeLogLevel) {
            std::string levelStr = logLevelToString(level);
            *m_logFile << levelStr << m_config.logSeparator;
            entrySize += static_cast<std::streamsize>(
                levelStr.size() + m_config.logSeparator.size());
        }

        // Write the main message
        *m_logFile << message;

        // Add task name if configured and provided
        std::string effectiveTaskName =
            taskName.empty() ? m_taskName : taskName;
        if (m_config.includeTaskName && !effectiveTaskName.empty()) {
            *m_logFile << " (" << effectiveTaskName << ")";
            entrySize +=
                static_cast<std::streamsize>(3 + effectiveTaskName.size());
        }

        // End the log entry
        *m_logFile << std::endl;
        entrySize += 1; // For newline

        // Update current file size
        m_currentFileSize += entrySize;

        // Flush if configured
        if (m_config.flushAfterEachWrite) {
            m_logFile->flush();
        }

        return true;

    } catch (const std::exception &e) {
        // Try to log the error to std::cerr
        std::cerr << "Error writing to log file: " << e.what() << std::endl;

        // Emit an error signal
        ArgumentPack args;
        args.add<std::string>(std::string("Error writing to log file: ") +
                              e.what());
        emit("fileError", args);

        return false;
    }
}

void FileLogger::cleanupOldLogFiles() {
    // Skip if max files is 0 (unlimited)
    if (m_config.maxFiles <= 0) {
        return;
    }

    try {
        // Get all log files
        auto logFiles = getLogFiles();

        // If we're over the limit, delete the oldest files
        if (logFiles.size() > static_cast<size_t>(m_config.maxFiles)) {
            // Skip the newest 'maxFiles' files and delete the rest
            for (size_t i = static_cast<size_t>(m_config.maxFiles);
                 i < logFiles.size(); ++i) {
                try {
                    std::filesystem::remove(logFiles[i]);
                } catch (const std::exception &e) {
                    std::cerr << "Error deleting old log file " << logFiles[i]
                              << ": " << e.what() << std::endl;
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error during log file cleanup: " << e.what() << std::endl;
    }
}

std::vector<std::filesystem::path> FileLogger::getLogFiles() const {
    std::vector<std::filesystem::path> logFiles;

    try {
        if (!std::filesystem::exists(m_config.logDirectory)) {
            return logFiles;
        }

        // Find all files in the log directory
        for (const auto &entry :
             std::filesystem::directory_iterator(m_config.logDirectory)) {
            if (entry.is_regular_file()) {
                logFiles.push_back(entry.path());
            }
        }

        // Sort by last modification time (newest first)
        std::sort(
            logFiles.begin(), logFiles.end(),
            [](const std::filesystem::path &a, const std::filesystem::path &b) {
                return std::filesystem::last_write_time(a) >
                       std::filesystem::last_write_time(b);
            });
    } catch (const std::exception &e) {
        std::cerr << "Error getting log files: " << e.what() << std::endl;
    }

    return logFiles;
}

void FileLogger::notifyRotationCallbacks(
    const std::filesystem::path &oldFilePath) {
    for (const auto &[id, callback] : m_rotationCallbacks) {
        try {
            callback(oldFilePath);
        } catch (const std::exception &e) {
            std::cerr << "Error in rotation callback: " << e.what()
                      << std::endl;
        }
    }
}