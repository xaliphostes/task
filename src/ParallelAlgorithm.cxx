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

#include <task/ParallelAlgorithm.h>
#include <future>
#include <thread>
#include <vector>

ParallelAlgorithm::ParallelAlgorithm() {
    // Create additional signals specific to parallel processing
    createDataSignal("job_started");
    createDataSignal("job_finished");
}

void ParallelAlgorithm::exec(const ArgumentPack &args) {
    if (m_jobs.empty()) {
        emitString("log", "No jobs to execute");
        emit("finished");
        return;
    }

    emit("started");
    emitString("log", "Starting parallel execution of " +
                          std::to_string(m_jobs.size()) + " jobs");

    // Create futures for each job
    std::vector<std::future<void>> futures;
    futures.reserve(m_jobs.size());

    // Launch each job in a separate thread
    for (size_t i = 0; i < m_jobs.size(); ++i) {
        const auto &job = m_jobs[i];
        futures.push_back(std::async(std::launch::async, [this, job, i]() {
            if (stopRequested()) {
                ArgumentPack jobArgs;
                jobArgs.add<size_t>(i);
                emit("job_started", jobArgs);
                emitString("warn", "Job " + std::to_string(i) +
                                       " skipped due to stop request");
                return;
            }

            try {
                // Signal that job is starting
                ArgumentPack startArgs;
                startArgs.add<size_t>(i);
                emit("job_started", startArgs);

                // Execute the job
                this->doJob(job);

                // Signal that job is finished
                ArgumentPack finishArgs;
                finishArgs.add<size_t>(i);
                finishArgs.add<bool>(true); // Success
                emit("job_finished", finishArgs);

                // Report progress
                float progress = static_cast<float>(i + 1) /
                                 static_cast<float>(m_jobs.size());
                reportProgress(progress);

            } catch (const std::exception &e) {
                // Create an ArgumentPack for the error message
                ArgumentPack errorArgs;
                errorArgs.add<std::string>("Job " + std::to_string(i) +
                                           " failed: " + e.what());
                emit("error", errorArgs);

                // Signal that job is finished with error
                ArgumentPack finishArgs;
                finishArgs.add<size_t>(i);
                finishArgs.add<bool>(false); // Failure
                emit("job_finished", finishArgs);
            }
        }));
    }

    // Wait for all jobs to complete
    for (auto &future : futures) {
        if (stopRequested()) {
            emitString("warn", "Execution stopped by user request");
            break;
        }
        future.wait();
    }

    // If we were requested to stop, some futures might still be running
    if (stopRequested()) {
        emitString("log", "Waiting for running jobs to complete...");

        // We still need to wait for futures that might be running
        for (auto &future : futures) {
            future.wait();
        }
    }

    emitString("log", "Parallel execution completed");
    emit("finished");
}