#pragma once

#include "../Algorithm.h"
#include <vector>
#include <future>
#include <memory>

class Runnable;

class ThreadPool: public Algorithm
{
public:
    explicit ThreadPool(bool verbose = true);
    ~ThreadPool();

    void add(std::unique_ptr<Runnable> runnable);

    template <typename T, typename... Args>
    T *createAndAdd(Args &&...args);

    unsigned int size() const;
    static unsigned int maxThreadCount();

protected:
    const std::vector<std::unique_ptr<Runnable>> &runnables() const;
    void exec(const Args &args = {}) override ;

private:
    std::vector<std::unique_ptr<Runnable>> runnables_;
    bool verbose_;
};

#include "ThreadPool.hxx"