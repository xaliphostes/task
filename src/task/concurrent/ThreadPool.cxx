#include <task/concurrent/ThreadPool.h>
#include <task/concurrent/Runnable.h>
#include <iostream>
#include <chrono>

unsigned int ThreadPool::maxThreadCount()
{
    return std::thread::hardware_concurrency();
}

ThreadPool::ThreadPool(bool verbose)
    : verbose_(verbose)
{
}

ThreadPool::~ThreadPool() = default; // Plus besoin de delete explicite avec unique_ptr

unsigned int ThreadPool::size() const
{
    return static_cast<unsigned int>(runnables_.size());
}

const std::vector<std::unique_ptr<Runnable>> &ThreadPool::runnables() const
{
    return runnables_;
}

void ThreadPool::add(std::unique_ptr<Runnable> runnable)
{
    runnables_.push_back(std::move(runnable));
}

void ThreadPool::exec(const std::vector<std::any> &args)
{
    std::vector<std::future<void>> futures;
    auto start = std::chrono::steady_clock::now();

    // Lance les threads en parallèle avec async
    for (const auto &runnable : runnables_)
    {
        futures.push_back(
            std::async(
                std::launch::async, // Force l'exécution parallèle
                &Runnable::run,
                runnable.get()));
    }

    // Attend la fin de tous les threads
    for (auto &future : futures)
    {
        future.get();
    }

    if (verbose_)
    {
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        std::cout << "Computation time on " << futures.size() << " threads: "
                  << std::chrono::duration<double, std::milli>(diff).count()
                  << " ms" << std::endl;
    }
}
