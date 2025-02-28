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

#include <condition_variable>
#include <functional>
#include <map>
#include <string>
#include <task/Algorithm.h>
#include <task/TaskQueue.h>
#include <task/concurrent/Runnable.h>
#include <thread>
#include <vector>

/**
 * Task entry containing a task and its associated metadata
 */
struct TaskEntry {
    std::shared_ptr<Runnable> task;
    TaskPriority priority;
    int64_t enqueueTime;
    std::string description;

    // Comparison operator for priority queue
    bool operator<(const TaskEntry &other) const {
        // First compare by priority
        if (priority != other.priority) {
            return priority >
                   other.priority; // Lower enum value = higher priority
        }
        // If priority is the same, use FIFO ordering
        return enqueueTime > other.enqueueTime;
    }
};

TaskQueue::TaskQueue(unsigned int maxConcurrentTasks, bool autoStart)
    : m_maxConcurrentTasks(maxConcurrentTasks), m_activeCount(0),
      m_isRunning(false), m_stopRequested(false) {

    // Create standard signals
    createDataSignal("taskEnqueued");
    createDataSignal("taskStarted");
    createDataSignal("taskCompleted");
    createDataSignal("taskFailed");
    createDataSignal("queueStats");

    // Auto-start if requested
    if (autoStart) {
        start();
    }
}

TaskQueue::~TaskQueue() { stop(); }

void TaskQueue::start() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isRunning) {
        return;
    }

    m_isRunning = true;
    m_stopRequested = false;

    lock.unlock();
    m_processingThread = std::thread(&TaskQueue::processQueue, this);

    emitString("log", "TaskQueue started");
    emit("started");
}

void TaskQueue::stop(bool wait) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_isRunning) {
        return;
    }

    m_stopRequested = true;
    lock.unlock();

    m_condition.notify_all();

    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }

    // If waiting for task completion
    if (wait) {
        std::unique_lock<std::mutex> activeLock(m_activeMutex);
        m_activeCondition.wait(activeLock,
                               [this] { return m_activeCount == 0; });
    }

    emitString("log", "TaskQueue stopped");
    emit("finished");
}

void TaskQueue::stopAll() {
    std::unique_lock<std::mutex> lock(m_activeMutex);
    for (auto &task : m_activeTasks) {
        task->requestStop();
    }
    lock.unlock();

    emitString("log", "Stop requested for all running tasks");
}

bool TaskQueue::enqueue(std::shared_ptr<Runnable> task, TaskPriority priority,
                        const std::string &description) {
    if (!task) {
        emitString("error", "Cannot enqueue null task");
        return false;
    }

    if (!m_isRunning) {
        emitString("warn", "TaskQueue is not running, cannot enqueue task");
        return false;
    }

    // Create task entry
    TaskEntry entry;
    entry.task = task;
    entry.priority = priority;
    entry.enqueueTime = getCurrentTimestamp();
    entry.description = description.empty()
                            ? "Task #" + std::to_string(m_taskCounter++)
                            : description;

    // Set up signal forwarding
    setupTaskSignals(task.get());

    // Add to queue
    std::unique_lock<std::mutex> lock(m_mutex);
    m_taskQueue.push(entry);
    lock.unlock();

    // Notify processing thread
    m_condition.notify_one();

    // Emit enqueued signal
    ArgumentPack args;
    args.add<std::string>(entry.description);
    args.add<int>(static_cast<int>(priority));
    emit("taskEnqueued", args);

    emitString("log", "Task enqueued: " + entry.description +
                          " (Priority: " + priorityToString(priority) + ")");

    // Update queue stats
    emitQueueStats();

    return true;
}

/**
 * Clear all pending tasks from the queue
 *
 * @return The number of tasks removed from the queue
 */
size_t TaskQueue::clearQueue() {
    std::unique_lock<std::mutex> lock(m_mutex);
    size_t count = m_taskQueue.size();

    // Create a temporary empty queue and swap
    std::priority_queue<TaskEntry> emptyQueue;
    std::swap(m_taskQueue, emptyQueue);

    lock.unlock();

    emitString("log", "Cleared " + std::to_string(count) + " tasks from queue");
    emitQueueStats();

    return count;
}

/**
 * Get the number of pending tasks in the queue
 *
 * @return The number of pending tasks
 */
size_t TaskQueue::pendingCount() const {
    auto self = const_cast<TaskQueue*>(this);
    std::unique_lock<std::mutex> lock(self->m_mutex);
    return m_taskQueue.size();
}

/**
 * Get the number of currently active tasks
 *
 * @return The number of active tasks
 */
size_t TaskQueue::activeCount() const {
    auto self = const_cast<TaskQueue*>(this);
    std::unique_lock<std::mutex> lock(self->m_activeMutex);
    return m_activeCount;
}

/**
 * Check if the queue is currently running
 *
 * @return True if the queue is running
 */
bool TaskQueue::isRunning() const { return m_isRunning; }

/**
 * Set the maximum number of concurrent tasks
 *
 * @param maxTasks The new maximum concurrent task count
 */
void TaskQueue::setMaxConcurrentTasks(unsigned int maxTasks) {
    if (maxTasks == 0) {
        emitString("warn",
                   "Cannot set max concurrent tasks to 0, using 1 instead");
        maxTasks = 1;
    }

    m_maxConcurrentTasks = maxTasks;
    emitString("log",
               "Max concurrent tasks set to " + std::to_string(maxTasks));

    // Wake up the processing thread to potentially execute more tasks
    m_condition.notify_one();
}

/**
 * Get the current maximum concurrent task setting
 *
 * @return The maximum concurrent task count
 */
unsigned int TaskQueue::getMaxConcurrentTasks() const {
    return m_maxConcurrentTasks;
}

void TaskQueue::processQueue() {
    emitString("log", "Task queue processor started");

    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // Wait until there are tasks in the queue and we have capacity to
        // run them, or until stop is requested
        m_condition.wait(lock, [this] {
            return m_stopRequested || (!m_taskQueue.empty() &&
                                       m_activeCount < m_maxConcurrentTasks);
        });

        // Check if we should exit
        if (m_stopRequested) {
            m_isRunning = false;
            break;
        }

        // Get the highest priority task
        TaskEntry entry = m_taskQueue.top();
        m_taskQueue.pop();

        // Update active task count
        {
            std::unique_lock<std::mutex> activeLock(m_activeMutex);
            m_activeCount++;
            m_activeTasks.insert(entry.task);
        }

        // Emit stats
        emitQueueStats();

        // Emit taskStarted signal
        ArgumentPack startArgs;
        startArgs.add<std::string>(entry.description);
        startArgs.add<int>(static_cast<int>(entry.priority));
        emit("taskStarted", startArgs);

        // Release lock before executing task
        lock.unlock();

        emitString("log", "Starting task: " + entry.description);

        // Execute the task asynchronously
        std::thread taskThread([this, entry]() {
            try {
                // Run the task
                entry.task->run();

                // Emit completion signal
                ArgumentPack completeArgs;
                completeArgs.add<std::string>(entry.description);
                completeArgs.add<int>(static_cast<int>(entry.priority));
                emit("taskCompleted", completeArgs);

                emitString("log", "Task completed: " + entry.description);
            } catch (const std::exception &e) {
                // Emit failure signal
                ArgumentPack failArgs;
                failArgs.add<std::string>(entry.description);
                failArgs.add<int>(static_cast<int>(entry.priority));
                failArgs.add<std::string>(e.what());
                emit("taskFailed", failArgs);

                emitString("error", "Task failed: " + entry.description +
                                        " - " + e.what());
            } catch (...) {
                // Emit failure signal for unknown exception
                ArgumentPack failArgs;
                failArgs.add<std::string>(entry.description);
                failArgs.add<int>(static_cast<int>(entry.priority));
                failArgs.add<std::string>("Unknown exception");
                emit("taskFailed", failArgs);

                emitString("error", "Task failed with unknown exception: " +
                                        entry.description);
            }

            // Update active task count and notify
            {
                std::unique_lock<std::mutex> activeLock(m_activeMutex);
                m_activeCount--;
                m_activeTasks.erase(entry.task);
                activeLock.unlock();
                m_activeCondition.notify_all();
            }

            // Notify queue processor to check for more tasks
            m_condition.notify_one();

            // Update stats
            emitQueueStats();
        });

        // Detach the thread to let it run independently
        taskThread.detach();
    }

    emitString("log", "Task queue processor stopped");
}

void TaskQueue::emitQueueStats() {
    size_t pending;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        pending = m_taskQueue.size();
    }

    size_t active;
    {
        std::unique_lock<std::mutex> lock(m_activeMutex);
        active = m_activeCount;
    }

    ArgumentPack args;
    args.add<size_t>(pending);                    // Pending tasks
    args.add<size_t>(active);                     // Active tasks
    args.add<unsigned int>(m_maxConcurrentTasks); // Max concurrent
    emit("queueStats", args);
}

void TaskQueue::setupTaskSignals(Runnable *task) {
    // Forward log signals
    task->connectData(
        "log", [this](const ArgumentPack &args) { this->emit("log", args); });

    task->connectData(
        "warn", [this](const ArgumentPack &args) { this->emit("warn", args); });

    task->connectData("error", [this](const ArgumentPack &args) {
        this->emit("error", args);
    });
}

int64_t TaskQueue::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string TaskQueue::priorityToString(TaskPriority priority) const {
    switch (priority) {
    case TaskPriority::CRITICAL:
        return "CRITICAL";
    case TaskPriority::HIGH:
        return "HIGH";
    case TaskPriority::NORMAL:
        return "NORMAL";
    case TaskPriority::LOW:
        return "LOW";
    case TaskPriority::BACKGROUND:
        return "BACKGROUND";
    default:
        return "UNKNOWN";
    }
}
