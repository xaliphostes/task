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
#include <thread>
#include <future>
#include <vector>
#include <iostream>

void ParallelAlgorithm::exec(const Args &args)
{
    if (m_jobs.empty())
    {
        emit("finished");
        return;
    }

    emit("started");

    // Création des futures pour chaque job
    std::vector<std::future<void>> futures;
    futures.reserve(m_jobs.size());

    // Lance chaque job dans un thread séparé
    for (const auto &job : m_jobs)
    {
        futures.push_back(std::async(std::launch::async, [this, job]()
                                     {
                try {
                    this->doJob(job);
                }
                catch (const std::exception& e) {
                    emit("error", Args{
                        std::string("Job failed: ") + e.what()
                    });
                } }));
    }

    // Attend que tous les jobs soient terminés
    for (auto &future : futures)
    {
        future.wait();
    }

    emit("finished");
}
