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

#include <task/FlowAlgorithm.h>

void FlowAlgorithm::addJob(const std::any &job) {
    m_jobs.push_back(job);

    // Emit a signal to indicate job has been added
    ArgumentPack args;
    args.add<size_t>(m_jobs.size());
    emitString("log",
               "Job added. Total jobs: " + std::to_string(m_jobs.size()));
}

void FlowAlgorithm::clearJobs() {
    size_t oldSize = m_jobs.size();
    m_jobs.clear();

    // Emit a signal that jobs have been cleared
    ArgumentPack args;
    args.add<size_t>(oldSize);
    emitString("log", "Cleared " + std::to_string(oldSize) + " jobs");
}
