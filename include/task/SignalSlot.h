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
#include "types.h"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// ----------------------------------------

class Connection {
  public:
    virtual void trigger(const Args &args) = 0;
    virtual ~Connection();
};

// ----------------------------------------

class FunctionConnection : public Connection {
  public:
    using Function = std::function<void(const Args &)>;

    explicit FunctionConnection(Function func);
    void trigger(const Args &args) override;

  private:
    Function m_func;
};

// ----------------------------------------

class SimpleFunctionConnection : public Connection {
  public:
    using Function = std::function<void()>;
    explicit SimpleFunctionConnection(Function func);
    void trigger(const Args &args) override;

  private:
    Function m_func;
};

// ----------------------------------------

class SignalSlot {
  public:
    SignalSlot(std::ostream & = std::cerr);

    void setOutputStream(std::ostream &);
    std::ostream &stream() const;

    void createSignal(const std::string &);

    void connect(const std::string &, std::function<void(const Args &)>);
    void connect(const std::string &, std::function<void()>);

    template <typename T>
    void connect(const std::string &, T *, void (T::*method)(const Args &));
    template <typename T>
    void connect(const std::string &, T *, void (T::*method)());

    void emit(const std::string &, const Args &args = {});

  protected:
    bool hasSignal(const std::string &) const;

  private:
    std::map<std::string, std::vector<std::shared_ptr<Connection>>> m_signals;
    std::reference_wrapper<std::ostream> m_output_stream;
};

// ----------------------------------------

#include "SignalSlot.hxx"