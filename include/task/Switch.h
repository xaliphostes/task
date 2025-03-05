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
#include <map>
#include <string>
#include <variant>

/**
 * A multi-way conditional task that executes other tasks based on a selector
 * value.
 *
 * The Switch task implements a multi-branch control flow mechanism in the task
 * framework. It evaluates a selector function and executes the corresponding
 * case task, or the default task if no matching case is found.
 * 
 * @code
 * // Create a Switch with a string selector
 * Switch stringSwitch([](const ArgumentPack& args) {
 *     return args.get<std::string>(0);  // Use first argument as case selector
 * });
 * 
 * // Add cases
 * stringSwitch.case_("start", startTask)
 *             .case_("process", processTask)
 *             .case_("stop", stopTask)
 *             .default_(errorTask);
 * 
 * // Execute based on a value
 * stringSwitch.execute(ArgumentPack("process"));  // Will execute processTask
 * 
 * // Integer-based switch
 * Switch intSwitch([](const ArgumentPack& args) {
 *     return args.get<int>(0);  // Use first argument as case selector
 * });
 * 
 * intSwitch.case_(1, task1)
 *          .case_(2, task2)
 *          .case_(3, task3)
 *          .default_(defaultTask);
 * @endcode
 */
class Switch : public Task {
  public:
    /**
     * Type definition for selector functions that return string keys.
     * The selector function can optionally receive parameters through an
     * ArgumentPack.
     */
    using StringSelectorFunction =
        std::function<std::string(const ArgumentPack &)>;

    /**
     * Type definition for selector functions that return integer values.
     * The selector function can optionally receive parameters through an
     * ArgumentPack.
     */
    using IntSelectorFunction = std::function<int(const ArgumentPack &)>;

    /**
     * Type for storing the variant of possible selector functions.
     */
    using SelectorFunction =
        std::variant<StringSelectorFunction, IntSelectorFunction>;

    /**
     * Constructor with string selector function.
     * @param selector The function that determines which case to execute
     */
    explicit Switch(StringSelectorFunction selector);

    /**
     * Constructor with integer selector function.
     * @param selector The function that determines which case to execute
     */
    explicit Switch(IntSelectorFunction selector);

    /**
     * Add a case branch for a string key.
     * @param caseValue The string value to match
     * @param task The task to execute for this case
     * @return A reference to this Switch task for method chaining
     */
    Switch &case_(const std::string &caseValue, Task *task);

    /**
     * Add a case branch for an integer key.
     * @param caseValue The integer value to match
     * @param task The task to execute for this case
     * @return A reference to this Switch task for method chaining
     */
    Switch &case_(int caseValue, Task *task);

    /**
     * Set the default task to execute when no cases match.
     * @param task The task to execute as default
     * @return A reference to this Switch task for method chaining
     */
    Switch &default_(Task *task);

    /**
     * Evaluate the selector and execute the appropriate task.
     * @param args Arguments that will be passed to the selector and tasks
     */
    void execute(const ArgumentPack &args = {});

    /**
     * Execute asynchronously and return a future.
     * @param args Arguments that will be passed to the selector and tasks
     * @return A std::future that can be used to wait for completion
     */
    std::future<void> executeAsync(const ArgumentPack &args = {});

  private:
    SelectorFunction m_selector;
    std::map<std::string, Task *> m_stringCases;
    std::map<int, Task *> m_intCases;
    Task *m_defaultTask = nullptr;
    bool m_isStringSelector;

    // Helper method to determine which task to execute
    Task *findTaskToExecute(const ArgumentPack &args);
};