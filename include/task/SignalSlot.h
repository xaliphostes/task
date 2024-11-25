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

using Args = std::vector<std::any>;

class Connection
{
public:
    virtual void trigger(const Args &args) = 0;
    virtual ~Connection();
};

class FunctionConnection : public Connection
{
public:
    explicit FunctionConnection(std::function<void(const Args &)> func);
    void trigger(const Args &args) override;

private:
    std::function<void(const Args &)> m_func;
};

class SimpleFunctionConnection : public Connection
{
public:
    explicit SimpleFunctionConnection(std::function<void()> func);
    void trigger(const Args &args) override;

private:
    std::function<void()> m_func;
};

class SignalSlot
{
public:
    void createSignal(const std::string &signal);
    void connect(const std::string &signal, std::function<void(const Args &)> slot);
    void connect(const std::string &signal, std::function<void()> slot);

    template <typename T>
    void connect(const std::string &signal, T *instance, void (T::*method)(const Args &));

    template <typename T>
    void connect(const std::string &signal, T *instance, void (T::*method)());

    void emit(const std::string &signal, const Args &args = {});

protected:
    bool hasSignal(const std::string &signal) const;

private:
    std::map<std::string, std::vector<std::shared_ptr<Connection>>> m_signals;
};

#include "SignalSlot.hxx"