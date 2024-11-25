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

#include <task/Task.h>
#include <task/Logger.h>

class ExampleTask : public Task {
public:
    ExampleTask() {
        // Création des signaux de journalisation
        Logger::createSignalsFor(this);
    }

    void doSomething() {
        emit("log", Args{std::string("Starting task...")});
        
        try {
            // Simulation d'une opération qui pourrait échouer
            if (rand() % 2) {
                throw std::runtime_error("Something went wrong!");
            }
            
            emit("log", Args{std::string("Task completed successfully")});
        }
        catch (const std::exception& e) {
            emit("error", Args{std::string(e.what())});
        }
    }

    void warnTest() {
        emit("warn", Args{std::string("This is a warning message")});
    }
};

// Exemple d'utilisation complète
int main() {
    // Création du logger et de la tâche
    auto logger = std::make_shared<Logger>(">>>");
    auto task = std::make_shared<ExampleTask>();

    // Connexion du logger à la tâche
    logger->connectAllSignalsTo(task.get());

    // Test avec plusieurs tâches
    std::vector<Task*> tasks = {task.get()};
    logger->connectAllSignalsTo(tasks);

    // Usage
    task->doSomething();
    task->warnTest();

    // Exemple with colors
    logger->log(Args{std::string("Information message")});
    logger->warn(Args{std::string("Warning message")});
    logger->error(Args{std::string("Error message")});
}
