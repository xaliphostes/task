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

class Logger {
public:
    void onLog(const std::vector<std::any>& args) {
        if (!args.empty()) {
            try {
                std::cout << "Log: " << std::any_cast<std::string>(args[0]) << std::endl;
            } catch (const std::bad_any_cast&) {
                std::cout << "Log: [invalid format]" << std::endl;
            }
        }
    }
};

int main() {
    Task task;
    Logger logger;

    // Connexion avec une méthode
    task.connect("log", &logger, &Logger::onLog);

    // Connexion avec une lambda
    task.connect("started", [](const std::vector<std::any>& args) {
        std::cout << "Task started!" << std::endl;
    });

    // Émission des signaux
    task.emit("started");
    task.emit("log", std::vector<std::any>{std::string("Task is running...")});

    return 0;
}