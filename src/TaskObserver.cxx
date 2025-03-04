/*
 * Copyright (c) 2024-now fmaerten@gmail.com
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

#include <memory>
#include <sstream>
#include <string>
#include <task/TaskObserver.h>
#include <typeinfo>
#include <vector>

TaskObserver::TaskObserver(const std::string &name) : m_name(name) {
    // Create signals for reporting
    createSignal("statsUpdated");
    createSignal("taskStarted");
    createSignal("taskFinished");
    createSignal("taskFailed");
}

bool TaskObserver::attach(Task *task, const std::string &taskName) {
    if (!task) {
        emitString("error", "Cannot attach observer to null task");
        return false;
    }

    // Generate a task name if not provided
    std::string name = taskName;
    if (name.empty()) {
        name = typeid(*task).name();
    }

    // Create a new stats entry or retrieve existing one
    auto &stats = m_taskStats[task];
    stats.taskName = name;
    stats.taskType = typeid(*task).name();

    // Connect to task signals
    ConnectionHandles handles;

    handles.push_back(task->connect(
        "started", [this, task]() { this->onTaskStarted(task); }));

    handles.push_back(task->connect(
        "finished", [this, task]() { this->onTaskFinished(task); }));

    handles.push_back(
        task->connect("error", [this, task](const ArgumentPack &args) {
            this->onTaskError(task, args);
        }));

    handles.push_back(
        task->connect("progress", [this, task](const ArgumentPack &args) {
            this->onTaskProgress(task, args);
        }));

    m_connections[task] = std::move(handles);

    emitString("log", "Observer attached to task: " + name);
    return true;
}

bool TaskObserver::detach(Task *task) {
    auto connIt = m_connections.find(task);
    if (connIt == m_connections.end()) {
        return false;
    }

    // Disconnect all connections
    auto &handles = connIt->second;
    for (auto &handle : handles) {
        handle->disconnect();
    }

    // Remove connections and stats
    m_connections.erase(task);
    m_taskStats.erase(task);

    emitString("log", "Observer detached from task");
    return true;
}

const TaskObserver::TaskStats *TaskObserver::getTaskStats(Task *task) const {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end()) {
        return nullptr;
    }
    return &(it->second);
}

/**
 * Get statistics for all tasks
 * @return A vector of task statistics
 */
std::vector<TaskObserver::TaskStats> TaskObserver::getAllTaskStats() const {
    std::vector<TaskObserver::TaskStats> result;
    result.reserve(m_taskStats.size());

    for (const auto &[task, stats] : m_taskStats) {
        result.push_back(stats);
    }

    return result;
}

bool TaskObserver::addCustomMetric(Task *task, const std::string &metricName,
                                   double value) {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end()) {
        return false;
    }

    it->second.customMetrics[metricName] = value;
    return true;
}

void TaskObserver::resetAllStats() {
    for (auto &[task, stats] : m_taskStats) {
        stats.executionCount = 0;
        stats.successCount = 0;
        stats.failureCount = 0;
        stats.totalExecutionTimeMs = 0;
        stats.minExecutionTimeMs = std::numeric_limits<int64_t>::max();
        stats.maxExecutionTimeMs = 0;
        stats.customMetrics.clear();
    }

    emitString("log", "All task statistics reset");
}

double TaskObserver::getAverageExecutionTime(Task *task) const {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end() || it->second.executionCount == 0) {
        return -1.0;
    }

    return static_cast<double>(it->second.totalExecutionTimeMs) /
           it->second.executionCount;
}

double TaskObserver::getSuccessRate(Task *task) const {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end() || it->second.executionCount == 0) {
        return -1.0;
    }

    return (static_cast<double>(it->second.successCount) /
            it->second.executionCount) *
           100.0;
}

std::string TaskObserver::generateSummaryReport() const {
    std::stringstream ss;
    ss << "=== Task Observer Summary Report: " << m_name << " ===\n";
    ss << "Total observed tasks: " << m_taskStats.size() << "\n\n";

    // Group tasks by type
    std::map<std::string, std::vector<const TaskStats *>> tasksByType;
    for (const auto &[task, stats] : m_taskStats) {
        tasksByType[stats.taskType].push_back(&stats);
    }

    // Report by task type
    for (const auto &[type, statsList] : tasksByType) {
        ss << "Task Type: " << type << "\n";
        ss << "  Instances: " << statsList.size() << "\n";

        int totalExecutions = 0;
        int totalSuccesses = 0;
        int totalFailures = 0;
        int64_t totalTimeMs = 0;

        for (const auto *stats : statsList) {
            totalExecutions += stats->executionCount;
            totalSuccesses += stats->successCount;
            totalFailures += stats->failureCount;
            totalTimeMs += stats->totalExecutionTimeMs;
        }

        if (totalExecutions > 0) {
            double avgTime = static_cast<double>(totalTimeMs) / totalExecutions;
            double successRate =
                (static_cast<double>(totalSuccesses) / totalExecutions) * 100.0;

            ss << "  Total executions: " << totalExecutions << "\n";
            ss << "  Success rate: " << successRate << "%\n";
            ss << "  Average execution time: " << avgTime << " ms\n";
        } else {
            ss << "  No executions recorded\n";
        }

        ss << "\n";
    }

    // Most executed tasks
    ss << "Top 5 most executed tasks:\n";
    std::vector<const TaskStats *> sortedByExecution;
    for (const auto &[task, stats] : m_taskStats) {
        sortedByExecution.push_back(&stats);
    }

    std::sort(sortedByExecution.begin(), sortedByExecution.end(),
              [](const TaskStats *a, const TaskStats *b) {
                  return a->executionCount > b->executionCount;
              });

    for (size_t i = 0; i < 5 && i < sortedByExecution.size(); ++i) {
        const auto *stats = sortedByExecution[i];
        ss << "  " << stats->taskName << ": " << stats->executionCount
           << " executions\n";
    }

    ss << "\n";

    // Slowest tasks
    ss << "Top 5 slowest tasks (by average execution time):\n";
    std::vector<const TaskStats *> sortedByTime;
    for (const auto &[task, stats] : m_taskStats) {
        if (stats.executionCount > 0) {
            sortedByTime.push_back(&stats);
        }
    }

    std::sort(sortedByTime.begin(), sortedByTime.end(),
              [](const TaskStats *a, const TaskStats *b) {
                  double avgA = static_cast<double>(a->totalExecutionTimeMs) /
                                a->executionCount;
                  double avgB = static_cast<double>(b->totalExecutionTimeMs) /
                                b->executionCount;
                  return avgA > avgB;
              });

    for (size_t i = 0; i < 5 && i < sortedByTime.size(); ++i) {
        const auto *stats = sortedByTime[i];
        double avgTime = static_cast<double>(stats->totalExecutionTimeMs) /
                         stats->executionCount;
        ss << "  " << stats->taskName << ": " << avgTime << " ms average\n";
    }

    return ss.str();
}

void TaskObserver::onTaskStarted(Task *task) {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end()) {
        return;
    }

    // Record start time
    auto &stats = it->second;
    m_taskStartTimes[task] = std::chrono::steady_clock::now();

    // Update last execution time
    stats.lastExecutionTime = std::chrono::system_clock::now();

    // Emit started signal
    ArgumentPack args;
    args.add<std::string>(stats.taskName);
    args.add<std::string>(stats.taskType);
    emit("taskStarted", args);
}

void TaskObserver::onTaskFinished(Task *task) {
    auto it = m_taskStats.find(task);
    auto timeIt = m_taskStartTimes.find(task);

    if (it == m_taskStats.end() || timeIt == m_taskStartTimes.end()) {
        return;
    }

    // Calculate execution time
    auto &stats = it->second;
    auto endTime = std::chrono::steady_clock::now();
    auto startTime = timeIt->second;
    auto execTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          endTime - startTime)
                          .count();

    // Update stats
    stats.executionCount++;
    stats.successCount++;
    stats.totalExecutionTimeMs += execTimeMs;
    stats.minExecutionTimeMs = std::min(stats.minExecutionTimeMs, execTimeMs);
    stats.maxExecutionTimeMs = std::max(stats.maxExecutionTimeMs, execTimeMs);

    // Clean up start time
    m_taskStartTimes.erase(task);

    // Emit finished signal
    ArgumentPack args;
    args.add<std::string>(stats.taskName);
    args.add<std::string>(stats.taskType);
    args.add<int64_t>(execTimeMs);
    args.add<bool>(true); // success
    emit("taskFinished", args);

    // Emit stats updated
    emitStatsUpdated(task, stats);
}

void TaskObserver::onTaskError(Task *task, const ArgumentPack &args) {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end()) {
        return;
    }

    // Update stats
    auto &stats = it->second;
    stats.failureCount++;

    // Get error message if available
    std::string errorMsg;
    if (!args.empty()) {
        try {
            errorMsg = args.get<std::string>(0);
        } catch (const std::bad_cast &) {
            errorMsg = "Unknown error";
        }
    }

    // Emit failed signal
    ArgumentPack failArgs;
    failArgs.add<std::string>(stats.taskName);
    failArgs.add<std::string>(stats.taskType);
    failArgs.add<std::string>(errorMsg);
    emit("taskFailed", failArgs);

    // Emit stats updated
    emitStatsUpdated(task, stats);
}

void TaskObserver::onTaskProgress(Task *task, const ArgumentPack &args) {
    auto it = m_taskStats.find(task);
    if (it == m_taskStats.end() || args.empty()) {
        return;
    }

    // Update progress
    auto &stats = it->second;
    try {
        stats.lastProgress = args.get<float>(0);
    } catch (const std::bad_cast &) {
        // Ignore invalid progress format
    }
}

void TaskObserver::emitStatsUpdated(Task *task, const TaskStats &stats) {
    ArgumentPack args;
    args.add<std::string>(stats.taskName);
    args.add<std::string>(stats.taskType);
    args.add<int>(stats.executionCount);
    args.add<int>(stats.successCount);
    args.add<int>(stats.failureCount);

    // Calculate average time
    double avgTime = 0.0;
    if (stats.executionCount > 0) {
        avgTime = static_cast<double>(stats.totalExecutionTimeMs) /
                  stats.executionCount;
    }
    args.add<double>(avgTime);

    emit("statsUpdated", args);
}
