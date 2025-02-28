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

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <task/Task.h>
#include <task/concurrent/Runnable.h>
#include <thread>

/**
 * Priority levels for tasks in the queue
 */
enum class TaskPriority {
    CRITICAL = 0,  // Highest priority, execute immediately
    HIGH = 1,      // High priority tasks
    NORMAL = 2,    // Normal priority tasks
    LOW = 3,       // Low priority, can wait
    BACKGROUND = 4 // Lowest priority, run when system is idle
};

struct TaskEntry;

/**
 * A priority-based task queue that schedules and executes tasks based on
 * their priority level and available resources.
 */
class TaskQueue : public Task {
  public:
    /**
     * Constructor for TaskQueue
     *
     * @param maxConcurrentTasks Maximum number of tasks that can run
     * concurrently
     * @param autoStart Whether to start the queue processing thread immediately
     */
    TaskQueue(unsigned int maxConcurrentTasks = 1, bool autoStart = true);

    /**
     * Destructor ensures the queue is properly stopped
     */
    ~TaskQueue();

    /**
     * Start the task queue processing thread
     */
    void start();

    /**
     * Stop the task queue processing thread
     *
     * @param wait Whether to wait for completion of running tasks
     */
    void stop(bool wait = true);

    /**
     * Stop all currently running tasks
     */
    void stopAll();

    /**
     * Enqueue a task with the specified priority
     *
     * @param task The task to enqueue
     * @param priority The priority level for this task
     * @param description Optional description for the task
     * @return True if the task was successfully enqueued
     */
    bool enqueue(std::shared_ptr<Runnable> task,
                 TaskPriority priority = TaskPriority::NORMAL,
                 const std::string &description = "");

    /**
     * Create a task of type T and enqueue it
     *
     * @param priority Priority level for the task
     * @param description Optional description for the task
     * @param args Arguments to forward to the constructor of T
     * @return A shared pointer to the created task
     */
    template <typename T, typename... Args>
    std::shared_ptr<T> createAndEnqueue(TaskPriority priority,
                                        const std::string &description,
                                        Args &&...args);

    /**
     * Clear all pending tasks from the queue
     *
     * @return The number of tasks removed from the queue
     */
    size_t clearQueue();

    /**
     * Get the number of pending tasks in the queue
     *
     * @return The number of pending tasks
     */
    size_t pendingCount() const;

    /**
     * Get the number of currently active tasks
     *
     * @return The number of active tasks
     */
    size_t activeCount() const;

    /**
     * Check if the queue is currently running
     *
     * @return True if the queue is running
     */
    bool isRunning() const;

    /**
     * Set the maximum number of concurrent tasks
     *
     * @param maxTasks The new maximum concurrent task count
     */
    void setMaxConcurrentTasks(unsigned int maxTasks);

    /**
     * Get the current maximum concurrent task setting
     *
     * @return The maximum concurrent task count
     */
    unsigned int getMaxConcurrentTasks() const;

  private:
    /**
     * Main queue processing function that runs in a separate thread
     */
    void processQueue();

    /**
     * Emit current queue statistics
     */
    void emitQueueStats();

    /**
     * Setup signal forwarding from a task to this queue
     */
    void setupTaskSignals(Runnable *task);

    /**
     * Get current timestamp in milliseconds
     */
    int64_t getCurrentTimestamp() const;

    /**
     * Convert priority enum to string representation
     */
    std::string priorityToString(TaskPriority priority) const;

    // Queue related members
    std::priority_queue<TaskEntry> m_taskQueue;
    std::mutex m_mutex;
    std::condition_variable m_condition;

    // Active task tracking
    std::atomic<size_t> m_activeCount;
    std::set<std::shared_ptr<Runnable>> m_activeTasks;
    std::mutex m_activeMutex;
    std::condition_variable m_activeCondition;

    // Thread management
    std::thread m_processingThread;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_stopRequested;

    // Configuration
    std::atomic<unsigned int> m_maxConcurrentTasks;

    // Task counter for generating unique IDs
    std::atomic<uint64_t> m_taskCounter{1};
};

#include "inline/TaskQueue.hxx"