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
#include <task/Logger.h>

class ExampleParallelAlgorithm : public ParallelAlgorithm {
public:
    void doJob(const std::any& job) override {
        try {
            // Conversion du job en string pour l'exemple
            std::string jobData = std::any_cast<std::string>(job);
            
            // Simulation d'un traitement
            emit("log", Args{
                std::string("Processing job: ") + jobData
            });
            
            // Simule un travail qui prend du temps
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            emit("log", Args{
                std::string("Completed job: ") + jobData
            });
        }
        catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid job data format");
        }
    }
};

// Exemple d'utilisation
int main() {
    // Création de l'algorithme
    auto algorithm = std::make_shared<ExampleParallelAlgorithm>();
    
    // Ajout d'un logger
    auto logger = std::make_shared<Logger>();
    logger->connectAllSignalsTo(algorithm.get());

    // Ajout des jobs
    algorithm->addJob(std::string("Job 1"));
    algorithm->addJob(std::string("Job 2"));
    algorithm->addJob(std::string("Job 3"));
    algorithm->addJob(std::string("Job 4"));

    // Exécution de l'algorithme
    auto future = algorithm->run();

    // Attente de la fin de l'exécution
    future.wait();
}