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
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

// -------------------------------------------------------

/**
 * A flexible argument container
 *
 * @code
 * // Verbose creation:
 * ArgumentPack args;
 * args.add<std::string>("data.csv");
 * args.add<float>(0.75f);
 * args.add<int>(10);
 *
 * // Direct creation:
 * ArgumentPack args("data.csv", 0.75f, 10);
 *
 * // Access values:
 * std::string filename = args.get<std::string>(0);
 * float threshold = args.get<float>(1);
 * int iterations = args.get<int>(2);
 * @endcode
 */
class ArgumentPack {
  public:
    // Default constructor
    ArgumentPack() = default;

    // Variadic template constructor to create with multiple arguments
    template <typename... Args> ArgumentPack(Args &&...args) {
        (add(std::forward<Args>(args)), ...);
    }

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

// -------------------------------------------------------

// Forward declaration
class ConnectionBase;
using ConnectionHandle = std::shared_ptr<ConnectionBase>;

// -------------------------------------------------------

// Base class for connections
class ConnectionBase {
  public:
    virtual ~ConnectionBase() = default;
    virtual void disconnect() = 0;
    virtual bool connected() const = 0;
};

// -------------------------------------------------------

// Signal interface class
class SignalBase {
  public:
    virtual ~SignalBase() = default;
    virtual void disconnectAll() = 0;
};

// -------------------------------------------------------

// Synchronization policy for signal emission
enum class SyncPolicy {
    Direct,  // Execute handlers directly in the emitting thread
    Blocking // Block until all handlers have completed
};

// -------------------------------------------------------

/**
 * Thread-safe unified signal class that can handle both parameterless and
 * parameter-based slots.
 */
class Signal : public SignalBase {
  public:
    using SimpleSlotFunction = std::function<void()>;
    using DataSlotFunction = std::function<void(const ArgumentPack &)>;

    // Connect any callable object
    template <typename Func> ConnectionHandle connect(Func &&slot);

    // Connect to member functions (no arguments)
    template <typename T>
    ConnectionHandle connect(T *instance, void (T::*method)());

    // Connect to member functions (with ArgumentPack)
    template <typename T>
    ConnectionHandle connect(T *instance,
                             void (T::*method)(const ArgumentPack &));

    // Emit without arguments
    void emit(SyncPolicy policy = SyncPolicy::Direct) const;

    // Emit with arguments
    void emit(const ArgumentPack &args,
              SyncPolicy policy = SyncPolicy::Direct) const;

    // Convenience method to emit with a string argument
    void emitString(const std::string &value,
                    SyncPolicy policy = SyncPolicy::Direct) const;

    // Clear all connections
    void disconnectAll() override;

  private:
    // Unified connection class
    class Connection : public ConnectionBase {
      public:
        Connection(Signal *signal, SimpleSlotFunction simpleSlot);
        Connection(Signal *signal, DataSlotFunction dataSlot);
        void disconnect() override;
        bool connected() const override;
        void call() const;
        void call(const ArgumentPack &args) const;

      private:
        Signal *m_signal;
        SimpleSlotFunction m_simpleSlot;
        DataSlotFunction m_dataSlot;
        std::atomic<bool> m_connected;
        bool m_takesArguments;
    };

    // Take a snapshot of active connections for thread-safe emission
    std::vector<std::shared_ptr<Connection>> getActiveConnections() const;

    mutable std::mutex m_connectionsMutex;
    mutable std::vector<std::weak_ptr<Connection>> m_connections;
};

// -------------------------------------------------------

/**
 * Thread-safe signal-slot system for decoupled component communication.
 *
 * Thread Safety Guarantees:
 * - Multiple threads can safely connect to and emit signals
 * - Signal emission uses a snapshot approach to avoid locking during callbacks
 * - Handlers are executed in the emitting thread by default
 * - Connections are thread-safe for creation and disconnection
 */
class SignalSlot {
  public:
    SignalSlot(std::ostream &output = std::cerr) : m_output_stream(output) {}

    virtual ~SignalSlot() { disconnectAllSignals(); }

    void setOutputStream(std::ostream &stream) {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        m_output_stream = stream;
    }

    std::ostream &stream() const {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        return m_output_stream;
    }

    // Create a signal (unified method for both types)
    bool createSignal(const std::string &name);

    // Check signal existence
    bool hasSignal(const std::string &name) const {
        std::lock_guard<std::mutex> lock(m_signalsMutex);
        return m_signals.find(name) != m_signals.end();
    }

    // Connect any callable to a signal
    template <typename Func>
    ConnectionHandle connect(const std::string &name, Func &&slot);

    // Connect member function to a signal (auto-detect argument requirements)
    template <typename T, typename Method>
    ConnectionHandle connect(const std::string &name, T *instance,
                             Method method);

    // Emit signals
    void emit(const std::string &name, SyncPolicy policy = SyncPolicy::Direct);
    void emit(const std::string &name, const ArgumentPack &args,
              SyncPolicy policy = SyncPolicy::Direct);
    void emitString(const std::string &name, const std::string &value,
                    SyncPolicy policy = SyncPolicy::Direct);

    // Disconnect all signals
    void disconnectAllSignals();

  private:
    // Get signal by name
    std::shared_ptr<Signal> getSignal(const std::string &name) const;

    mutable std::mutex m_signalsMutex;
    std::map<std::string, std::shared_ptr<Signal>> m_signals;

    mutable std::mutex m_streamMutex;
    std::reference_wrapper<std::ostream> m_output_stream;
};

// -------------------------------------------------------

#include "inline/SignalSlot.hxx"