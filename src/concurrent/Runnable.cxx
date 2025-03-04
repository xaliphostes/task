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

#include <task/concurrent/Runnable.h>

Runnable::Runnable() : m_isRunning(false), m_stopRequested(false) {
    // Create progress signal
    createSignal("progress");
}

void Runnable::run() {
    if (m_isRunning) {
        emitString("warn", "Task is already running");
        return;
    }

    m_stopRequested = false;
    m_isRunning = true;

    // Emit started signal
    emit("started");

    try {
        // Run the implementation
        runImpl();
    } catch (const std::exception &e) {
        // Report error if an exception occurs
        emitString("error", e.what());
    } catch (...) {
        // Handle unknown exceptions
        emitString("error", "Unknown exception during task execution");
    }

    // Mark task as completed
    m_isRunning = false;

    // Emit finished signal
    emit("finished");
}

std::future<void> Runnable::runAsync() {
    return std::async(std::launch::async, [this]() { this->run(); });
}

bool Runnable::isRunning() const { return m_isRunning; }

void Runnable::requestStop() { m_stopRequested = true; }

bool Runnable::stopRequested() const { return m_stopRequested; }

void Runnable::reportProgress(float progress) {
    // Clamp progress value between 0 and 1
    float clampedProgress = std::max(0.0f, std::min(1.0f, progress));

    // Create arguments with the progress value
    ArgumentPack args;
    args.add<float>(clampedProgress);

    // Emit the progress signal
    emit("progress", args);
}