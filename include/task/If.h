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
#include <functional>
#include <future>
#include <memory>

/**
 * A conditional task that executes other tasks based on a condition.
 *
 * The If task implements a conditional control flow mechanism in the task
 * framework. It evaluates a condition and executes either the "then" task
 * or the "else" task based on the result.
 */
class If : public Task {
  public:
    /**
     * Type definition for condition functions.
     * Condition can optionally receive parameters through an ArgumentPack.
     */
    using ConditionFunction = std::function<bool(const ArgumentPack &)>;

    /**
     * Constructor with condition function.
     * @param condition The function that determines which branch to execute
     */
    explicit If(ConditionFunction condition);

    /**
     * Set a task to execute when the condition is true.
     * @param task The task to execute in the "then" branch
     * @return A reference to this If task for method chaining
     */
    If &then(Task* task);

    /**
     * Set a task to execute when the condition is false.
     * @param task The task to execute in the "else" branch
     * @return A reference to this If task for method chaining
     */
    If &else_(Task* task);

    /**
     * Evaluate the condition and execute the appropriate task.
     * @param args Arguments that will be passed to the condition and tasks
     */
    void execute(const ArgumentPack &args = {});

    /**
     * Execute asynchronously and return a future.
     * @param args Arguments that will be passed to the condition and tasks
     * @return A std::future that can be used to wait for completion
     */
    std::future<void> executeAsync(const ArgumentPack &args = {});

  private:
    ConditionFunction m_condition;
    std::shared_ptr<Task> m_thenTask;
    std::shared_ptr<Task> m_elseTask;
};