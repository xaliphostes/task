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

#include <task/For.h>

class LoopObserver {
public:
    void onTick(const std::vector<std::any>& args) {
        if (args.empty()) return;

        try {
            const auto& tickData = std::any_cast<std::map<std::string, std::any>>(args[0]);
            int current = std::any_cast<int>(tickData.at("current"));
            int start = std::any_cast<int>(tickData.at("start"));
            int stop = std::any_cast<int>(tickData.at("stop"));

            std::cout << "Loop progress: " << current 
                     << " (Start: " << start 
                     << ", Stop: " << stop << ")" 
                     << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing tick data: " << e.what() << std::endl;
        }
    }
};

// Exemple d'utilisation
int main() {
    // Création de la boucle avec des paramètres
    For loop({0, 5, 1});  // start=0, stop=5, step=1

    // Création de l'observateur
    LoopObserver observer;

    // Connexion du signal tick à l'observateur
    loop.connect("tick", &observer, &LoopObserver::onTick);

    // Version synchrone
    loop.start();

    // Version asynchrone
    auto future = loop.startAsync();
    future.wait();

    // Modification des paramètres
    loop.set(ForParameters{10, 0, -1});

    // Utilisation des setters individuels
    loop.setStartValue(0);
    loop.setStopValue(100);
    loop.setStepValue(2);

    // Démarrage avec les nouveaux paramètres
    loop.start();
}