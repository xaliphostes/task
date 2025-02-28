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
#include "../Algorithm.h"
#include <future>
#include <memory>
#include <thread>
#include <vector>

class Runnable;

/**
 * A thread pool that executes multiple Runnable tasks in parallel.
 * Inherits from Algorithm to support the signal-slot system.
 */
class ThreadPool : public Algorithm {
  public:
    /**
     * Constructor for ThreadPool
     * @param verbose Whether to output execution time information
     */
    explicit ThreadPool(bool verbose = true);

    /**
     * Destructor
     */
    ~ThreadPool() = default;

    /**
     * Add a runnable task to the thread pool
     * @param runnable A unique_ptr to a Runnable instance
     */
    void add(std::unique_ptr<Runnable> runnable);

    /**
     * Create a new runnable of type T and add it to the pool
     * @param args Arguments to forward to the constructor of T
     * @return A raw pointer to the created runnable (owned by the pool)
     */
    template <typename T, typename... Args> T *createAndAdd(Args &&...args);

    /**
     * Get the number of tasks in the pool
     * @return The number of tasks
     */
    unsigned int size() const;

    /**
     * Get the maximum number of concurrent threads supported by the hardware
     * @return The maximum thread count
     */
    static unsigned int maxThreadCount();

    /**
     * Execute all the tasks in the pool
     * @param args Arguments for execution
     */
    void exec(const ArgumentPack &args = {}) override;

    /**
     * Connect a logger to all runnables in the pool
     * @param logger The logger to connect
     */
    void connectLoggerToAll(Task *logger);

    /**
     * Stop all running tasks
     */
    void stopAll();

  protected:
    /**
     * Get the list of runnables
     * @return A const reference to the vector of runnables
     */
    const std::vector<std::unique_ptr<Runnable>> &runnables() const;

  private:
    std::vector<std::unique_ptr<Runnable>> m_runnables;
    bool m_verbose;
};

// Include the template implementation
#include "inline/ThreadPool.hxx"