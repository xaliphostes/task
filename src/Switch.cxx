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

#include <sstream>
#include <task/Switch.h>
#include <task/Algorithm.h>

Switch::Switch(StringSelectorFunction selector)
    : m_selector(std::move(selector)), m_isStringSelector(true) {
    // Create signals for switch control flow
    createSignal("caseSelected");
    createSignal("defaultSelected");
    createSignal("noMatchFound");
}

Switch::Switch(IntSelectorFunction selector)
    : m_selector(std::move(selector)), m_isStringSelector(false) {
    // Create signals for switch control flow
    createSignal("caseSelected");
    createSignal("defaultSelected");
    createSignal("noMatchFound");
}

Switch &Switch::case_(const std::string &caseValue, Task *task) {
    if (!m_isStringSelector) {
        emitString("warn", "Adding string case to an integer selector Switch");
    }

    if (!task) {
        emitString("warn", "Null task provided for case '" + caseValue + "'");
        return *this;
    }

    m_stringCases[caseValue] = task;
    return *this;
}

Switch &Switch::case_(int caseValue, Task *task) {
    if (m_isStringSelector) {
        emitString("warn", "Adding integer case to a string selector Switch");
    }

    if (!task) {
        std::stringstream ss;
        ss << "Null task provided for case " << caseValue;
        emitString("warn", ss.str());
        return *this;
    }

    m_intCases[caseValue] = task;
    return *this;
}

Switch &Switch::default_(Task *task) {
    if (!task) {
        emitString("warn", "Null task provided for default case");
        return *this;
    }

    m_defaultTask = task;
    return *this;
}

Task *Switch::findTaskToExecute(const ArgumentPack &args) {
    try {
        if (m_isStringSelector) {
            std::string selectedCase =
                std::get<StringSelectorFunction>(m_selector)(args);

            // Emit the selected case
            ArgumentPack caseArgs;
            caseArgs.add<std::string>(selectedCase);

            auto it = m_stringCases.find(selectedCase);
            if (it != m_stringCases.end()) {
                emit("caseSelected", caseArgs);
                return it->second;
            }
        } else {
            int selectedCase = std::get<IntSelectorFunction>(m_selector)(args);

            // Emit the selected case
            ArgumentPack caseArgs;
            caseArgs.add<int>(selectedCase);

            auto it = m_intCases.find(selectedCase);
            if (it != m_intCases.end()) {
                emit("caseSelected", caseArgs);
                return it->second;
            }
        }

        // No matching case found, use default
        if (m_defaultTask) {
            emit("defaultSelected");
            return m_defaultTask;
        }

        // No default task either
        emit("noMatchFound");
        emitString("warn", "No matching case or default task found");
        return nullptr;
    } catch (const std::exception &e) {
        emitString("error",
                   "Exception in selector function: " + std::string(e.what()));
        return nullptr;
    } catch (...) {
        emitString("error", "Unknown exception in selector function");
        return nullptr;
    }
}

void Switch::execute(const ArgumentPack &args) {
    emit("started");

    Task *taskToExecute = findTaskToExecute(args);
    if (taskToExecute) {
        try {
            // Connect the task signals to our signals
            ConnectionHandle startedConn =
                taskToExecute->connect("started", [this]() {
                    this->emitString("log", "Case task started");
                });

            ConnectionHandle finishedConn =
                taskToExecute->connect("finished", [this]() {
                    this->emitString("log", "Case task finished");
                });

            ConnectionHandle errorConn = taskToExecute->connect(
                "error", [this](const ArgumentPack &args) {
                    this->emit("error", args);
                });

            // Execute the selected task
            // Note: This is where you would define how tasks are executed in
            // your framework This might need to be adapted to your actual task
            // execution mechanism
            if (auto algorithm = dynamic_cast<Algorithm *>(taskToExecute)) {
                algorithm->exec(args);
            } else {
                // For non-algorithm tasks, you might have a different execution
                // method
                emitString("warn", "Selected task is not an Algorithm and "
                                   "cannot be executed directly");
            }
        } catch (const std::exception &e) {
            emitString("error", "Exception during task execution: " +
                                    std::string(e.what()));
        } catch (...) {
            emitString("error", "Unknown exception during task execution");
        }
    }

    emit("finished");
}

std::future<void> Switch::executeAsync(const ArgumentPack &args) {
    auto argsClone = args.clone();
    return std::async(std::launch::async,
                      [this, argsClone = std::move(argsClone)]() {
                          this->execute(argsClone);
                      });
}