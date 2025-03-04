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
#include <optional>
#include <task/ProgressMonitor.h>

// Class to monitor and respond to progress events
ProgressMonitor::ProgressMonitor() { createSignal("summary"); }

void ProgressMonitor::onProgress(const ArgumentPack &args) {
    float progress = args.get<float>(0);
    m_lastProgress = progress;

    // Log significant progress milestones
    if (progress >= m_nextMilestone) {
        emitString("log", "Progress milestone reached: " +
                              std::to_string(static_cast<int>(progress * 100)) +
                              "%");
        m_nextMilestone += 0.25f; // Next milestone at 25%, 50%, 75%, 100%
    }
}

void ProgressMonitor::onTaskStarted() {
    m_startedTasks++;
    emitString("log", "Task started. " + std::to_string(m_startedTasks) +
                          " tasks running");
}

void ProgressMonitor::onTaskFinished() {
    m_completedTasks++;
    emitString("log", "Task finished. " + std::to_string(m_completedTasks) +
                          " tasks completed");

    // Emit summary when all tasks are done
    if (m_taskCount > 0 && m_completedTasks == m_taskCount) {
        ArgumentPack summaryArgs;
        summaryArgs.add<int>(m_taskCount);
        summaryArgs.add<int>(m_completedTasks);
        emit("summary", summaryArgs);
    }
}

void ProgressMonitor::setTaskCount(int count) {
    m_taskCount = count;
    m_startedTasks = 0;
    m_completedTasks = 0;
    m_nextMilestone = 0.25f;
}
