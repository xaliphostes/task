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
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * A flexible argument container
 *
 * @code
 * ArgumentPack args;
 * args.add<std::string>("data.csv");
 * args.add<float>(0.75f);
 * args.add<int>(10);
 *
 * std::string filename = args.get<std::string>(0);
 * float threshold = args.get<float>(1);
 * int iterations = args.get<int>(2);
 * @endcode
 */
class ArgumentPack {
  public:
    ArgumentPack() = default;

    // Allow move operations, but disable copy operations
    ArgumentPack(ArgumentPack &&other) noexcept = default;
    ArgumentPack &operator=(ArgumentPack &&other) noexcept = default;
    ArgumentPack(const ArgumentPack &) = delete;
    ArgumentPack &operator=(const ArgumentPack &) = delete;

    template <typename T> void add(T value);
    template <typename T> T &get(size_t index);
    template <typename T> const T &get(size_t index) const;
    size_t size() const { return m_args.size(); }
    bool empty() const { return m_args.empty(); }
    template <typename T> T &operator[](size_t index);
    template <typename T> const T &operator[](size_t index) const;

    std::string getTypeName(size_t index) const {
        if (index >= m_args.size()) {
            throw std::out_of_range("Argument index out of range");
        }
        return m_args[index]->typeName();
    }

    // Deep copy method (use when needed instead of copy constructor)
    ArgumentPack clone() const {
        ArgumentPack copy;
        for (size_t i = 0; i < m_args.size(); ++i) {
            copy.m_args.push_back(m_args[i]->clone());
        }
        return copy;
    }

  private:
    struct BaseHolder {
        virtual ~BaseHolder() = default;
        virtual std::string typeName() const = 0;
        virtual std::unique_ptr<BaseHolder> clone() const = 0;
    };

    template <typename T> struct Holder : BaseHolder {
        explicit Holder(T value) : m_value(std::move(value)) {}
        std::string typeName() const override { return typeid(T).name(); }
        std::unique_ptr<BaseHolder> clone() const override {
            return std::make_unique<Holder<T>>(m_value);
        }
        T m_value;
    };

    std::vector<std::unique_ptr<BaseHolder>> m_args;
};

// Forward declaration
class ConnectionBase;
using ConnectionHandle = std::shared_ptr<ConnectionBase>;

// Base class for connections
class ConnectionBase {
  public:
    virtual ~ConnectionBase() = default;
    virtual void disconnect() = 0;
    virtual bool connected() const = 0;
};

// Signal interface class
class SignalBase {
  public:
    virtual ~SignalBase() = default;
    virtual void disconnectAll() = 0;
};

// Only two types of signals: with or without args
enum class SignalType {
    NO_ARGS,  // Signal with no arguments (simple notification)
    WITH_ARGS // Signal with ArgumentPack arguments
};

// Simple signal with no arguments
class SimpleSignal : public SignalBase {
  public:
    using SlotFunction = std::function<void()>;

    // Connect a function to this signal
    ConnectionHandle connect(SlotFunction &&slot);

    // Connect a member function
    template <typename T>
    ConnectionHandle connect(T *instance, void (T::*method)());

    // Emit the signal
    void emit() const;

    // Clear all connections
    void disconnectAll() override;

  private:
    // Concrete connection class
    class Connection : public ConnectionBase {
      public:
        Connection(SimpleSignal *signal, SlotFunction slot);
        void disconnect() override;
        bool connected() const override;
        void call() const;

      private:
        SimpleSignal *m_signal;
        SlotFunction m_slot;
        bool m_connected;
    };

    mutable std::vector<std::weak_ptr<Connection>> m_connections;
};

// Signal with ArgumentPack
class DataSignal : public SignalBase {
  public:
    using SlotFunction = std::function<void(const ArgumentPack &)>;

    // Connect a function to this signal
    ConnectionHandle connect(SlotFunction &&slot);

    // Connect a member function
    template <typename T>
    ConnectionHandle connect(T *instance,
                             void (T::*method)(const ArgumentPack &));

    // Emit the signal
    void emit(const ArgumentPack &args) const;

    // Emit with a single string argument (convenience method)
    void emitString(const std::string &value) const;

    // Clear all connections
    void disconnectAll() override;

  private:
    // Concrete connection class
    class Connection : public ConnectionBase {
      public:
        Connection(DataSignal *signal, SlotFunction slot);
        void disconnect() override;
        bool connected() const override;
        void call(const ArgumentPack &args) const;

      private:
        DataSignal *m_signal;
        SlotFunction m_slot;
        bool m_connected;
    };

    mutable std::vector<std::weak_ptr<Connection>> m_connections;
};

// SignalSlot class that manages signals by name
class SignalSlot {
  public:
    SignalSlot(std::ostream &output = std::cerr) : m_output_stream(output) {}

    virtual ~SignalSlot() {
        for (auto &[name, signal_ptr] : m_signals) {
            signal_ptr->disconnectAll();
        }
        m_signals.clear();
    }

    void setOutputStream(std::ostream &stream) { m_output_stream = stream; }
    std::ostream &stream() const { return m_output_stream; }

    // Create signals
    bool createSimpleSignal(const std::string &name);
    bool createDataSignal(const std::string &name);

    // Check signal existence
    bool hasSignal(const std::string &name) const {
        return m_signals.find(name) != m_signals.end();
    }

    // Get signal type
    SignalType getSignalType(const std::string &name) const {
        if (!hasSignal(name))
            return SignalType::NO_ARGS; // Default
        return m_signal_types.at(name);
    }

    // Connect to simple signal (no args)
    ConnectionHandle connectSimple(const std::string &name,
                                   std::function<void()> slot);

    template <typename T>
    ConnectionHandle connectSimple(const std::string &name, T *instance,
                                   void (T::*method)());

    // Connect to data signal (with ArgumentPack)
    ConnectionHandle
    connectData(const std::string &name,
                std::function<void(const ArgumentPack &)> slot);

    template <typename T>
    ConnectionHandle connectData(const std::string &name, T *instance,
                                 void (T::*method)(const ArgumentPack &));

    // Emit signals
    void emit(const std::string &name); // For simple signals
    void emit(const std::string &name,
              const ArgumentPack &args); // For data signals

    // Convenience method to emit a signal with a single string
    void emitString(const std::string &name, const std::string &value);

  private:
    // Get signal by name and type
    std::shared_ptr<SimpleSignal> getSimpleSignal(const std::string &name);
    std::shared_ptr<DataSignal> getDataSignal(const std::string &name);

    std::map<std::string, std::shared_ptr<SignalBase>> m_signals;
    std::map<std::string, SignalType> m_signal_types;
    std::reference_wrapper<std::ostream> m_output_stream;
};

#include "inline/SignalSlot.hxx"