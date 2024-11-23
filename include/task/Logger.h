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

#pragma once
#include "Task.h"
#include <string>
#include <vector>
#include <iostream>

class Logger : public Task {
public:
    explicit Logger(const std::string& prefix = ">>") ;
    virtual ~Logger() = default;

    void log(const std::vector<std::any>& args) ;
    void warn(const std::vector<std::any>& args) ;
    void error(const std::vector<std::any>& args) ;

    // Méthode statique pour créer les signaux pour une tâche
    static void createSignalsFor(Task* task) ;

    // Connecter tous les signaux à une tâche
    void connectAllSignalsTo(Task* task) ;
    void connectAllSignalsTo(const std::vector<Task*>& tasks) ;

private:
    std::string m_prefix;
};
