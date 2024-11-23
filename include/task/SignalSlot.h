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
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <iostream>
#include <any>

class Connection {
public:
    virtual void trigger(const std::vector<std::any>& args) = 0;
    virtual ~Connection() = default;
};

class FunctionConnection : public Connection {
public:
    explicit FunctionConnection(std::function<void(const std::vector<std::any>&)> func) 
        : m_func(std::move(func)) {}

    void trigger(const std::vector<std::any>& args) override {
        m_func(args);
    }

private:
    std::function<void(const std::vector<std::any>&)> m_func;
};

class MethodConnection : public Connection {
public:
    template<typename T>
    MethodConnection(T* receiver, void (T::*method)(const std::vector<std::any>&)) {
        m_func = [receiver, method](const std::vector<std::any>& args) {
            (receiver->*method)(args);
        };
    }

    void trigger(const std::vector<std::any>& args) override {
        m_func(args);
    }

private:
    std::function<void(const std::vector<std::any>&)> m_func;
};

class SignalSlot {
public:
    void createSignal(const std::string& signal);
    
    template<typename Func>
    void connect(const std::string& signal, Func&& slot) {
        if (!hasSignal(signal)) {
            std::cerr << "Signal '" << signal << "' not found" << std::endl;
            return;
        }
        
        auto connection = std::make_shared<FunctionConnection>(
            std::forward<Func>(slot)
        );
        m_signals[signal].push_back(connection);
    }

    template<typename T>
    void connect(const std::string& signal, T* receiver, void (T::*method)(const std::vector<std::any>&)) {
        if (!hasSignal(signal)) {
            std::cerr << "Signal '" << signal << "' not found" << std::endl;
            return;
        }

        auto connection = std::make_shared<MethodConnection>(receiver, method);
        m_signals[signal].push_back(connection);
    }

    void emit(const std::string& signal, const std::vector<std::any>& args = {});

protected:
    bool hasSignal(const std::string& signal) const;

private:
    std::map<std::string, std::vector<std::shared_ptr<Connection>>> m_signals;
};