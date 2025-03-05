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

#include <future>
#include <sstream>
#include <task/Algorithm.h>
#include <task/concurrent/Runnable.h>
#include <task/If.h>

If::If(ConditionFunction condition) : m_condition(std::move(condition)) {
    // Create signals specific to conditional execution
    createSignal("conditionEvaluated");
    createSignal("branchSelected");
    createSignal("thenExecuted");
    createSignal("elseExecuted");
    createSignal("noBranchExecuted");
}

If &If::then(Task* task) {
    m_thenTask = std::shared_ptr<Task>(task);

    // Forward relevant signals from the then task
    if (m_thenTask) {
        // Forward log signals
        m_thenTask->connect("log", [this](const ArgumentPack &args) {
            if (!args.empty()) {
                try {
                    std::string message = "then: " + args.get<std::string>(0);
                    ArgumentPack logArgs;
                    logArgs.add<std::string>(message);
                    this->emit("log", logArgs);
                } catch (const std::exception &) {
                    // Handle potential type conversion errors
                }
            }
        });

        // Forward error signals
        m_thenTask->connect("error", [this](const ArgumentPack &args) {
            this->emit("error", args);
        });
    }

    return *this;
}

If &If::else_(Task* task) {
    m_elseTask = std::shared_ptr<Task>(task);

    // Forward relevant signals from the else task
    if (m_elseTask) {
        // Forward log signals
        m_elseTask->connect("log", [this](const ArgumentPack &args) {
            if (!args.empty()) {
                try {
                    std::string message = "else: " + args.get<std::string>(0);
                    ArgumentPack logArgs;
                    logArgs.add<std::string>(message);
                    this->emit("log", logArgs);
                } catch (const std::exception &) {
                    // Handle potential type conversion errors
                }
            }
        });

        // Forward error signals
        m_elseTask->connect("error", [this](const ArgumentPack &args) {
            this->emit("error", args);
        });
    }

    return *this;
}

void If::execute(const ArgumentPack &args) {
    emit("started");

    try {
        // Evaluate the condition
        bool conditionResult = m_condition(args);
        emit("conditionEvaluated");

        // Emit the branch selection result
        ArgumentPack branchArgs;
        branchArgs.add<bool>(conditionResult);
        branchArgs.add<std::string>(conditionResult ? "then" : "else");
        emit("branchSelected", branchArgs);

        // Log the branch selection
        std::stringstream ss;
        ss << "Condition evaluated to " << (conditionResult ? "true" : "false")
           << ", executing " << (conditionResult ? "then" : "else")
           << " branch";
        emitString("log", ss.str());

        if (conditionResult) {
            // Execute the "then" branch if it exists
            if (m_thenTask) {
                // Forward arguments to the task
                if (m_thenTask->hasSignal("started")) {
                    m_thenTask->emit("started");
                }

                // Execute any task-specific logic
                // Note: We're using dynamic casting here to handle different
                // task types
                if (auto algorithm =
                        dynamic_cast<Algorithm *>(m_thenTask.get())) {
                    algorithm->exec(args);
                } else if (auto runnable =
                               dynamic_cast<Runnable *>(m_thenTask.get())) {
                    runnable->run();
                }

                if (m_thenTask->hasSignal("finished")) {
                    m_thenTask->emit("finished");
                }

                emit("thenExecuted");
            } else {
                emitString("warn", "No task defined for the 'then' branch");
                emit("noBranchExecuted");
            }
        } else {
            // Execute the "else" branch if it exists
            if (m_elseTask) {
                // Forward arguments to the task
                if (m_elseTask->hasSignal("started")) {
                    m_elseTask->emit("started");
                }

                // Execute any task-specific logic
                if (auto algorithm =
                        dynamic_cast<Algorithm *>(m_elseTask.get())) {
                    algorithm->exec(args);
                } else if (auto runnable =
                               dynamic_cast<Runnable *>(m_elseTask.get())) {
                    runnable->run();
                }

                if (m_elseTask->hasSignal("finished")) {
                    m_elseTask->emit("finished");
                }

                emit("elseExecuted");
            } else {
                // No else branch defined, just log a message
                emitString("log", "No task defined for the 'else' branch, "
                                  "nothing to execute");
                emit("noBranchExecuted");
            }
        }
    } catch (const std::exception &e) {
        // Handle any exceptions that occurred during execution
        std::string errorMsg = "Error in If task: ";
        errorMsg += e.what();
        emitString("error", errorMsg);
    }

    emit("finished");
}

std::future<void> If::executeAsync(const ArgumentPack &args) {
    auto argsCopy = args.clone();
    return std::async(
        std::launch::async,
        [this, argsCopy = std::move(argsCopy)]() { this->execute(argsCopy); });
}