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
#include "../Task.h"
#include <future>

/**
 * A base class for tasks that can be executed by the ThreadPool.
 * Uses the SignalSlot system to report execution status.
 */
class Runnable : public Task {
  public:
    Runnable();
    virtual ~Runnable() = default;

    /**
     * Execute the runnable task, emitting started/finished signals.
     */
    void run();

    /**
     * Executes the task asynchronously and returns a future.
     * @return A std::future that can be used to wait for completion.
     */
    std::future<void> runAsync();

    /**
     * Check if the task is currently running.
     * @return True if the task is running, false otherwise.
     */
    bool isRunning() const;

    /**
     * Request the task to stop if possible.
     */
    void requestStop();

    /**
     * Check if a stop has been requested.
     * @return True if stop was requested, false otherwise.
     */
    bool stopRequested() const;

  protected:
    /**
     * Implementation of the task logic.
     * Must be implemented by subclasses.
     */
    virtual void runImpl() = 0;

    /**
     * Report progress of the current operation.
     * @param progress Value between 0.0 and 1.0 indicating completion
     * percentage.
     */
    void reportProgress(float progress);

  private:
    bool m_isRunning;
    bool m_stopRequested;
};