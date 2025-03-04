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

#include <chrono>
#include <iostream>
#include <sstream>
#include <task/concurrent/Runnable.h>
#include <task/concurrent/ThreadPool.h>

unsigned int ThreadPool::maxThreadCount() {
    return std::thread::hardware_concurrency();
}

ThreadPool::ThreadPool(bool verbose) : m_verbose(verbose) {
    // Create a data signal for thread pool statistics
    createSignal("stats");
}

unsigned int ThreadPool::size() const {
    return static_cast<unsigned int>(m_runnables.size());
}

const std::vector<std::unique_ptr<Runnable>> &ThreadPool::runnables() const {
    return m_runnables;
}

void ThreadPool::add(std::unique_ptr<Runnable> runnable) {
    if (runnable) {
        // Forward log signals from the runnable to this ThreadPool using
        // lambdas
        runnable->connect("log", [this](const ArgumentPack &args) {
            this->emit("log", args);
        });
        runnable->connect("warn", [this](const ArgumentPack &args) {
            this->emit("warn", args);
        });
        runnable->connect("error", [this](const ArgumentPack &args) {
            this->emit("error", args);
        });

        // Add the runnable to our collection
        m_runnables.push_back(std::move(runnable));

        // Emit information about the new size
        std::stringstream ss;
        ss << "Added runnable. Pool size: " << m_runnables.size();
        emitString("log", ss.str());
    }
}

void ThreadPool::exec(const ArgumentPack &args) {
    if (m_runnables.empty()) {
        emitString("warn", "ThreadPool is empty, nothing to execute");
        return;
    }

    std::vector<std::future<void>> futures;
    auto start = std::chrono::steady_clock::now();

    unsigned int taskCount = m_runnables.size();

    // Report starting information
    std::stringstream ss;
    ss << "Starting execution of " << taskCount << " tasks";
    emitString("log", ss.str());

    // Start progress reporting
    float progressStep = 1.0f / static_cast<float>(taskCount);
    reportProgress(0.0f);

    // Launch tasks in parallel with async
    for (const auto &runnable : m_runnables) {
        futures.push_back(
            std::async(std::launch::async, &Runnable::run, runnable.get()));
    }

    // Wait for all tasks to complete
    for (size_t i = 0; i < futures.size(); ++i) {
        futures[i].get();
        // Update progress after each task completes
        reportProgress((i + 1) * progressStep);
    }

    auto end = std::chrono::steady_clock::now();
    auto diffMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();

    // Create stats information
    ArgumentPack statsArgs;
    statsArgs.add<int64_t>(diffMs);         // Execution time in ms
    statsArgs.add<unsigned int>(taskCount); // Number of executed tasks
    emit("stats", statsArgs);

    if (m_verbose) {
        std::stringstream ss;
        ss << "Executed " << taskCount << " tasks in " << diffMs << " ms ("
           << static_cast<double>(diffMs) / taskCount
           << " ms per task average)";
        emitString("log", ss.str());
    }
}

void ThreadPool::connectLoggerToAll(Task *logger) {
    if (!logger)
        return;

    // Use lambda functions to properly forward signals to the logger

    // Connect the logger to the ThreadPool
    connect("log", [logger](const ArgumentPack &args) {
        logger->emit("log", args);
    });
    connect("warn", [logger](const ArgumentPack &args) {
        logger->emit("warn", args);
    });
    connect("error", [logger](const ArgumentPack &args) {
        logger->emit("error", args);
    });

    // Connect the logger to each Runnable
    for (const auto &runnable : m_runnables) {
        runnable->connect("log", [logger](const ArgumentPack &args) {
            logger->emit("log", args);
        });
        runnable->connect("warn", [logger](const ArgumentPack &args) {
            logger->emit("warn", args);
        });
        runnable->connect("error", [logger](const ArgumentPack &args) {
            logger->emit("error", args);
        });
    }
}

void ThreadPool::stopAll() {
    for (const auto &runnable : m_runnables) {
        if (runnable->isRunning()) {
            runnable->requestStop();
        }
    }
    emitString("log", "Stop requested for all running tasks");
}