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

// Type traits to detect if a function accepts ArgumentPack
namespace detail {

// Primary template - assumes not compatible with ArgumentPack
template <typename, typename = void>
struct takes_argument_pack : std::false_type {};

// Specialization for functions that can be called with ArgumentPack
template <typename F>
struct takes_argument_pack<F, std::void_t<decltype(std::declval<F>()(
                                  std::declval<const ArgumentPack &>()))>>
    : std::true_type {};

} // namespace detail

//---------------------
// ArgumentPack Implementation
//---------------------

template <typename T> void ArgumentPack::add(T value) {
    m_args.push_back(std::make_unique<Holder<T>>(std::move(value)));
}

template <typename T> T &ArgumentPack::get(size_t index) {
    if (index >= m_args.size()) {
        throw std::out_of_range("Argument index out of range");
    }
    auto *holder = dynamic_cast<Holder<T> *>(m_args[index].get());
    if (!holder) {
        throw std::bad_cast();
    }
    return holder->m_value;
}

template <typename T> const T &ArgumentPack::get(size_t index) const {
    if (index >= m_args.size()) {
        throw std::out_of_range("Argument index out of range");
    }
    auto *holder = dynamic_cast<Holder<T> *>(m_args[index].get());
    if (!holder) {
        throw std::bad_cast();
    }
    return holder->m_value;
}

template <typename T> T &ArgumentPack::operator[](size_t index) {
    return get<T>(index);
}

template <typename T> const T &ArgumentPack::operator[](size_t index) const {
    return get<T>(index);
}

//---------------------
// Signal Implementation
//---------------------

inline Signal::Connection::Connection(Signal *signal,
                                      SimpleSlotFunction simpleSlot)
    : m_signal(signal), m_simpleSlot(std::move(simpleSlot)),
      m_dataSlot(nullptr), m_connected(true), m_takesArguments(false) {}

inline Signal::Connection::Connection(Signal *signal, DataSlotFunction dataSlot)
    : m_signal(signal), m_simpleSlot(nullptr), m_dataSlot(std::move(dataSlot)),
      m_connected(true), m_takesArguments(true) {}

inline void Signal::Connection::disconnect() {
    bool expected = true;
    if (m_connected.compare_exchange_strong(expected, false)) {
        m_signal = nullptr;
    }
}

inline bool Signal::Connection::connected() const { return m_connected.load(); }

inline void Signal::Connection::call() const {
    if (m_connected.load()) {
        if (m_takesArguments) {
            // Call data slot with empty ArgumentPack
            ArgumentPack emptyArgs;
            m_dataSlot(emptyArgs);
        } else if (m_simpleSlot) {
            // Call simple slot
            m_simpleSlot();
        }
    }
}

inline void Signal::Connection::call(const ArgumentPack &args) const {
    if (m_connected.load()) {
        if (m_takesArguments && m_dataSlot) {
            // Call data slot with provided args
            m_dataSlot(args);
        } else if (m_simpleSlot) {
            // Call simple slot (ignoring arguments)
            m_simpleSlot();
        }
    }
}

template <typename Func> inline ConnectionHandle Signal::connect(Func &&slot) {
    // Determine if the slot takes an ArgumentPack
    if constexpr (detail::takes_argument_pack<Func>::value) {
        // Create data slot
        auto dataSlot = [f = std::forward<Func>(slot)](
                            const ArgumentPack &args) { f(args); };
        auto connection =
            std::make_shared<Connection>(this, std::move(dataSlot));

        // Thread-safe connection storage
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections.push_back(connection);
        return connection;
    } else {
        // Create simple slot
        auto simpleSlot = [f = std::forward<Func>(slot)]() { f(); };
        auto connection =
            std::make_shared<Connection>(this, std::move(simpleSlot));

        // Thread-safe connection storage
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections.push_back(connection);
        return connection;
    }
}

template <typename T>
inline ConnectionHandle Signal::connect(T *instance, void (T::*method)()) {
    if (!instance || !method) {
        return nullptr;
    }

    auto simpleSlot = [instance, method]() { (instance->*method)(); };

    auto connection = std::make_shared<Connection>(this, std::move(simpleSlot));

    // Thread-safe connection storage
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.push_back(connection);
    return connection;
}

template <typename T>
inline ConnectionHandle
Signal::connect(T *instance, void (T::*method)(const ArgumentPack &)) {
    if (!instance || !method) {
        return nullptr;
    }

    auto dataSlot = [instance, method](const ArgumentPack &args) {
        (instance->*method)(args);
    };

    auto connection = std::make_shared<Connection>(this, std::move(dataSlot));

    // Thread-safe connection storage
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.push_back(connection);
    return connection;
}

inline std::vector<std::shared_ptr<Signal::Connection>>
Signal::getActiveConnections() const {
    std::vector<std::shared_ptr<Connection>> activeConnections;

    // Thread-safe access to connections
    std::lock_guard<std::mutex> lock(m_connectionsMutex);

    // Create a snapshot of active connections
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        auto connection = it->lock();
        if (connection && connection->connected()) {
            activeConnections.push_back(connection);
            ++it;
        } else {
            // Clean up dead connections
            it = m_connections.erase(it);
        }
    }

    return activeConnections;
}

inline void Signal::emit(SyncPolicy policy) const {
    // Get a snapshot of active connections to avoid locking during callbacks
    auto activeConnections = getActiveConnections();

    // Execute all callbacks according to policy
    if (policy == SyncPolicy::Direct) {
        // Call handlers directly in this thread
        for (const auto &connection : activeConnections) {
            connection->call();
        }
    } else if (policy == SyncPolicy::Blocking) {
        // This is already blocking since we execute in the current thread
        for (const auto &connection : activeConnections) {
            connection->call();
        }
    }
}

inline void Signal::emit(const ArgumentPack &args, SyncPolicy policy) const {
    // Get a snapshot of active connections to avoid locking during callbacks
    auto activeConnections = getActiveConnections();

    // Execute all callbacks according to policy
    if (policy == SyncPolicy::Direct) {
        // Call handlers directly in this thread
        for (const auto &connection : activeConnections) {
            connection->call(args);
        }
    } else if (policy == SyncPolicy::Blocking) {
        // This is already blocking since we execute in the current thread
        for (const auto &connection : activeConnections) {
            connection->call(args);
        }
    }
}

inline void Signal::emitString(const std::string &value,
                               SyncPolicy policy) const {
    ArgumentPack args;
    args.add<std::string>(value);
    emit(args, policy);
}

inline void Signal::disconnectAll() {
    std::vector<std::shared_ptr<Connection>> connections;

    // Safely get all connections
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);

        // Create strong references to all connections before disconnecting
        for (auto &weakConnection : m_connections) {
            if (auto connection = weakConnection.lock()) {
                connections.push_back(connection);
            }
        }

        // Clear the connections vector
        m_connections.clear();
    }

    // Disconnect all connections (now that we're not holding the lock)
    for (auto &connection : connections) {
        connection->disconnect();
    }
}

//---------------------
// SignalSlot Implementation
//---------------------

inline bool SignalSlot::createSignal(const std::string &name) {
    std::lock_guard<std::mutex> lock(m_signalsMutex);

    if (m_signals.find(name) != m_signals.end()) {
        std::lock_guard<std::mutex> streamLock(m_streamMutex);
        m_output_stream.get()
            << "Signal '" << name << "' already exists" << std::endl;
        return false;
    }

    m_signals[name] = std::make_shared<Signal>();
    return true;
}

template <typename Func>
inline ConnectionHandle SignalSlot::connect(const std::string &name,
                                            Func &&slot) {
    auto signal = getSignal(name);
    if (!signal) {
        return nullptr;
    }
    return signal->connect(std::forward<Func>(slot));
}

template <typename T, typename Method>
inline ConnectionHandle SignalSlot::connect(const std::string &name,
                                            T *instance, Method method) {
    auto signal = getSignal(name);
    if (!signal) {
        return nullptr;
    }

    // Use if constexpr with type traits to determine the correct overload
    if constexpr (std::is_same_v<Method, void (T::*)(const ArgumentPack &)> ||
                  std::is_same_v<Method,
                                 void (T::*)(const ArgumentPack &) const>) {
        return signal->connect(instance, method);
    } else if constexpr (std::is_same_v<Method, void (T::*)()> ||
                         std::is_same_v<Method, void (T::*)() const>) {
        return signal->connect(instance, method);
    } else {
        // If neither signature matches, provide a helpful compile error
        static_assert(
            std::is_same_v<Method, void (T::*)(const ArgumentPack &)> ||
                std::is_same_v<Method, void (T::*)()>,
            "Method must be either void(T::*)() or void(T::*)(const "
            "ArgumentPack&)");
        return nullptr;
    }
}

inline void SignalSlot::emit(const std::string &name, SyncPolicy policy) {
    auto signal = getSignal(name);
    if (!signal) {
        return;
    }
    signal->emit(policy);
}

inline void SignalSlot::emit(const std::string &name, const ArgumentPack &args,
                             SyncPolicy policy) {
    auto signal = getSignal(name);
    if (!signal) {
        return;
    }
    signal->emit(args, policy);
}

inline void SignalSlot::emitString(const std::string &name,
                                   const std::string &value,
                                   SyncPolicy policy) {
    auto signal = getSignal(name);
    if (!signal) {
        return;
    }
    signal->emitString(value, policy);
}

inline std::shared_ptr<Signal>
SignalSlot::getSignal(const std::string &name) const {
    std::lock_guard<std::mutex> lock(m_signalsMutex);

    auto it = m_signals.find(name);
    if (it == m_signals.end()) {
        std::lock_guard<std::mutex> streamLock(m_streamMutex);
        m_output_stream.get()
            << "Signal '" << name << "' not found" << std::endl;
        return nullptr;
    }

    return it->second;
}

inline void SignalSlot::disconnectAllSignals() {
    std::vector<std::shared_ptr<Signal>> signals;

    // Get all signals while holding the lock
    {
        std::lock_guard<std::mutex> lock(m_signalsMutex);
        for (const auto &[name, signal] : m_signals) {
            signals.push_back(signal);
        }
        m_signals.clear();
    }

    // Disconnect all signals (without holding the lock)
    for (auto &signal : signals) {
        signal->disconnectAll();
    }
}
