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

#pragma once
#include "Task.h"
#include <chrono>
#include <map>

/**
 * A class that collects statistics about task execution in the system.
 * It can be attached to tasks to monitor their execution times, success/failure
 * rates, and other metrics.
 */
class TaskObserver : public Task {
  public:
    /**
     * Statistics structure for a single task
     */
    struct TaskStats {
        std::string taskName;
        std::string taskType;
        int executionCount = 0;
        int successCount = 0;
        int failureCount = 0;
        int64_t totalExecutionTimeMs = 0;
        int64_t minExecutionTimeMs = std::numeric_limits<int64_t>::max();
        int64_t maxExecutionTimeMs = 0;
        std::chrono::system_clock::time_point lastExecutionTime;
        double lastProgress = 0.0;

        // Task-specific metrics can be added here
        std::map<std::string, double> customMetrics;
    };

    /**
     * Constructor for TaskObserver
     * @param name The name of this observer instance
     */
    explicit TaskObserver(const std::string &name = "TaskObserver");

    /**
     * Attach this observer to a task
     * @param task The task to observe
     * @param taskName An optional name for the task (defaults to typeid name)
     * @return True if successfully attached, false otherwise
     */
    bool attach(Task *, const std::string &taskName = "");

    /**
     * Detach this observer from a task
     * @param task The task to detach from
     * @return True if successfully detached, false if the task wasn't observed
     */
    bool detach(Task *);

    /**
     * Get statistics for a specific task
     * @param task The task to get statistics for
     * @return The task statistics or nullptr if the task is not observed
     */
    const TaskStats *getTaskStats(Task *) const;

    /**
     * Get statistics for all tasks
     * @return A vector of task statistics
     */
    std::vector<TaskStats> getAllTaskStats() const;

    /**
     * Add a custom metric to a task's statistics
     * @param task The task to add the metric to
     * @param metricName The name of the metric
     * @param value The metric value
     * @return True if the metric was added, false if the task is not observed
     */
    bool addCustomMetric(Task *, const std::string &metricName, double value);

    /**
     * Reset statistics for all tasks
     */
    void resetAllStats();

    /**
     * Get the average execution time for a task
     * @param task The task to get the average time for
     * @return The average execution time in milliseconds or -1 if the task
     * hasn't executed
     */
    double getAverageExecutionTime(Task *) const;

    /**
     * Get the success rate for a task
     * @param task The task to get the success rate for
     * @return The success rate as a percentage (0-100) or -1 if the task hasn't
     * executed
     */
    double getSuccessRate(Task *) const;

    /**
     * Generate a summary report of all task statistics
     * @return A formatted string containing the summary
     */
    std::string generateSummaryReport() const;

  private:
    // Signal connection handles for a task
    using ConnectionHandles = std::vector<ConnectionHandle>;

    // Event handlers
    void onTaskStarted(Task *task);
    void onTaskFinished(Task *task);
    void onTaskError(Task *task, const ArgumentPack &args);
    void onTaskProgress(Task *task, const ArgumentPack &args);
    void emitStatsUpdated(Task *task, const TaskStats &stats);

    std::string m_name;
    std::map<Task *, TaskStats> m_taskStats;
    std::map<Task *, ConnectionHandles> m_connections;
    std::map<Task *, std::chrono::steady_clock::time_point> m_taskStartTimes;
};