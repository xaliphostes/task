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

    // Utilise unique_ptr pour une meilleure gestion de la mémoire
    void add(std::unique_ptr<Runnable> runnable);

    // Ajout d'une méthode template pour créer et ajouter directement
    template <typename T, typename... Args>
    T *createAndAdd(Args &&...args)
    {
        auto runnable = std::make_unique<T>(std::forward<Args>(args)...);
        T *ptr = runnable.get();
        add(std::move(runnable));
        return ptr;
    }

    unsigned int size() const;
    static unsigned int maxThreadCount();

protected:
    const std::vector<std::unique_ptr<Runnable>> &runnables() const;
    void exec(const std::vector<std::any> &args = {}) override ;

private:
    std::vector<std::unique_ptr<Runnable>> runnables_;
    bool verbose_;
};
