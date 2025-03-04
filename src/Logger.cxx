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
#include <memory>
#include <task/Logger.h>

Logger::Logger(const std::string &prefix) : m_prefix(prefix) {}

// Méthodes de journalisation
void Logger::log(const ArgumentPack &args) {
    if (!args.empty()) {
        try {
            std::cout << m_prefix << " " << args.get<std::string>(0)
                      << std::endl;
        } catch (const std::bad_cast &) {
            std::cout << m_prefix << " [invalid format]" << std::endl;
        }
    }
}

void Logger::warn(const ArgumentPack &args) {
    if (!args.empty()) {
        try {
            std::cerr << "\033[33m" << m_prefix
                      << " WARNING: " << args.get<std::string>(0) << "\033[0m"
                      << std::endl;
        } catch (const std::bad_cast &) {
            std::cerr << "\033[33m" << m_prefix
                      << " WARNING: [invalid format]\033[0m" << std::endl;
        }
    }
}

void Logger::error(const ArgumentPack &args) {
    if (!args.empty()) {
        try {
            std::cerr << "\033[31m" << m_prefix
                      << " ERROR: " << args.get<std::string>(0) << "\033[0m"
                      << std::endl;
        } catch (const std::bad_cast &) {
            std::cerr << "\033[31m" << m_prefix
                      << " ERROR: [invalid format]\033[0m" << std::endl;
        }
    }
}

// Méthode statique pour créer les signaux pour une tâche
void Logger::createSignalsFor(Task *task) {
    if (!task)
        return;

    const std::vector<std::string> signals = {"log", "warn", "error"};
    for (const auto &signal : signals) {
        task->createSignal(signal);
    }
}

// Connecter tous les signaux à une tâche
void Logger::connectAllSignalsTo(Task *task) {
    if (!task)
        return;

    // Connexion des signaux individuels
    task->connect("log", this, &Logger::log);
    task->connect("warn", this, &Logger::warn);
    task->connect("error", this, &Logger::error);
}

// Surcharge pour connecter à un vecteur de tâches
void Logger::connectAllSignalsTo(const std::vector<Task *> &tasks) {
    for (auto task : tasks) {
        connectAllSignalsTo(task);
    }
}