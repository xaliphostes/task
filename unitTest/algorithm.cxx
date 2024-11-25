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

#include <iostream>
#include <any>
#include <task/Task.h>
#include <task/Algorithm.h>

class ExampleAlgorithm : public Algorithm {
public:
    void exec(const Args& args = {}) override {
        // Exemple d'implémentation
        emit("log", Args{std::string("Starting algorithm execution")});
        
        // Simulation d'un traitement long
        for (int i = 0; i < 100; ++i) {
            if (stopRequested()) {
                emit("log", Args{std::string("Algorithm stopped by user")});
                return;
            }

            // Faire quelque chose...
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            emit("log", Args{
                std::string("Processing: ") + std::to_string(i) + "%"
            });
        }
        
        emit("log", Args{std::string("Algorithm completed")});
    }
};

// Exemple d'utilisation
int main() {
    ExampleAlgorithm algo;
    
    // Connexion des signaux
    algo.connect("started", [](const Args& args) {
        std::cout << "Algorithm started" << std::endl;
    });

    algo.connect("finished", [](const Args& args) {
        std::cout << "Algorithm finished" << std::endl;
    });

    algo.connect("log", [](const Args& args) {
        if (!args.empty()) {
            try {
                std::cout << std::any_cast<std::string>(args[0]) << std::endl;
            } catch (const std::bad_any_cast&) {
                std::cout << "Invalid log format" << std::endl;
            }
        }
    });

    // Lancement de l'algorithme de manière asynchrone
    auto future = algo.run();

    // Possibilité d'arrêter l'algorithme
    std::this_thread::sleep_for(std::chrono::seconds(2));
    algo.stop();

    // Attente de la fin de l'algorithme
    future.wait();
}
