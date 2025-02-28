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
// SimpleSignal Implementation
//---------------------

inline ConnectionHandle SimpleSignal::connect(SlotFunction &&slot) {
    auto connection =
        std::make_shared<Connection>(this, std::forward<SlotFunction>(slot));
    m_connections.push_back(connection);
    return connection;
}

template <typename T>
inline ConnectionHandle SimpleSignal::connect(T *instance,
                                              void (T::*method)()) {
    return connect([instance, method]() { (instance->*method)(); });
}

inline void SimpleSignal::emit() const {
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        auto connection = it->lock();
        if (connection && connection->connected()) {
            connection->call();
            ++it;
        } else {
            it = m_connections.erase(it);
        }
    }
}

inline void SimpleSignal::disconnectAll() {
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        auto connection = it->lock();
        if (connection) {
            connection->disconnect();
        }
        it = m_connections.erase(it);
    }
}

// SimpleSignal::Connection implementation
inline SimpleSignal::Connection::Connection(SimpleSignal *signal,
                                            SlotFunction slot)
    : m_signal(signal), m_slot(std::move(slot)), m_connected(true) {}

inline void SimpleSignal::Connection::disconnect() {
    m_connected = false;
    m_signal = nullptr;
}

inline bool SimpleSignal::Connection::connected() const { return m_connected; }

inline void SimpleSignal::Connection::call() const {
    if (m_connected && m_slot) {
        m_slot();
    }
}

//---------------------
// DataSignal Implementation
//---------------------

inline ConnectionHandle DataSignal::connect(SlotFunction &&slot) {
    auto connection =
        std::make_shared<Connection>(this, std::forward<SlotFunction>(slot));
    m_connections.push_back(connection);
    return connection;
}

template <typename T>
inline ConnectionHandle
DataSignal::connect(T *instance, void (T::*method)(const ArgumentPack &)) {
    return connect([instance, method](const ArgumentPack &args) {
        (instance->*method)(args);
    });
}

inline void DataSignal::emit(const ArgumentPack &args) const {
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        auto connection = it->lock();
        if (connection && connection->connected()) {
            connection->call(args);
            ++it;
        } else {
            it = m_connections.erase(it);
        }
    }
}

inline void DataSignal::emitString(const std::string &value) const {
    ArgumentPack args;
    args.add<std::string>(value);
    emit(args);
}

inline void DataSignal::disconnectAll() {
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        auto connection = it->lock();
        if (connection) {
            connection->disconnect();
        }
        it = m_connections.erase(it);
    }
}

// DataSignal::Connection implementation
inline DataSignal::Connection::Connection(DataSignal *signal, SlotFunction slot)
    : m_signal(signal), m_slot(std::move(slot)), m_connected(true) {}

inline void DataSignal::Connection::disconnect() {
    m_connected = false;
    m_signal = nullptr;
}

inline bool DataSignal::Connection::connected() const { return m_connected; }

inline void DataSignal::Connection::call(const ArgumentPack &args) const {
    if (m_connected && m_slot) {
        m_slot(args);
    }
}

//---------------------
// SignalSlot Implementation
//---------------------

inline bool SignalSlot::createSimpleSignal(const std::string &name) {
    if (hasSignal(name)) {
        stream() << "Signal '" << name << "' already exists" << std::endl;
        return false;
    }

    auto signal = std::make_shared<SimpleSignal>();
    m_signals[name] = signal;
    m_signal_types[name] = SignalType::NO_ARGS;
    return true;
}

inline bool SignalSlot::createDataSignal(const std::string &name) {
    if (hasSignal(name)) {
        stream() << "Signal '" << name << "' already exists" << std::endl;
        return false;
    }

    auto signal = std::make_shared<DataSignal>();
    m_signals[name] = signal;
    m_signal_types[name] = SignalType::WITH_ARGS;
    return true;
}

inline ConnectionHandle SignalSlot::connectSimple(const std::string &name,
                                                  std::function<void()> slot) {
    auto signal = getSimpleSignal(name);
    if (!signal) {
        return nullptr;
    }
    return signal->connect(std::move(slot));
}

template <typename T>
inline ConnectionHandle SignalSlot::connectSimple(const std::string &name,
                                                  T *instance,
                                                  void (T::*method)()) {
    auto signal = getSimpleSignal(name);
    if (!signal) {
        return nullptr;
    }
    return signal->connect(instance, method);
}

inline ConnectionHandle
SignalSlot::connectData(const std::string &name,
                        std::function<void(const ArgumentPack &)> slot) {
    auto signal = getDataSignal(name);
    if (!signal) {
        return nullptr;
    }
    return signal->connect(std::move(slot));
}

template <typename T>
inline ConnectionHandle
SignalSlot::connectData(const std::string &name, T *instance,
                        void (T::*method)(const ArgumentPack &)) {
    auto signal = getDataSignal(name);
    if (!signal) {
        return nullptr;
    }
    return signal->connect(instance, method);
}

inline void SignalSlot::emit(const std::string &name) {
    auto signal = getSimpleSignal(name);
    if (!signal) {
        stream() << "Simple signal '" << name << "' not found or type mismatch"
                 << std::endl;
        return;
    }
    signal->emit();
}

inline void SignalSlot::emit(const std::string &name,
                             const ArgumentPack &args) {
    auto signal = getDataSignal(name);
    if (!signal) {
        stream() << "Data signal '" << name << "' not found or type mismatch"
                 << std::endl;
        return;
    }
    signal->emit(args);
}

inline void SignalSlot::emitString(const std::string &name,
                                   const std::string &value) {
    auto signal = getDataSignal(name);
    if (!signal) {
        stream() << "Data signal '" << name << "' not found or type mismatch"
                 << std::endl;
        return;
    }
    signal->emitString(value);
}

inline std::shared_ptr<SimpleSignal>
SignalSlot::getSimpleSignal(const std::string &name) {
    if (!hasSignal(name)) {
        stream() << "Signal '" << name << "' not found" << std::endl;
        return nullptr;
    }

    if (m_signal_types[name] != SignalType::NO_ARGS) {
        stream() << "Type mismatch for signal '" << name << "'" << std::endl;
        return nullptr;
    }

    return std::static_pointer_cast<SimpleSignal>(m_signals[name]);
}

inline std::shared_ptr<DataSignal>
SignalSlot::getDataSignal(const std::string &name) {
    if (!hasSignal(name)) {
        stream() << "Signal '" << name << "' not found" << std::endl;
        return nullptr;
    }

    if (m_signal_types[name] != SignalType::WITH_ARGS) {
        stream() << "Type mismatch for signal '" << name << "'" << std::endl;
        return nullptr;
    }

    return std::static_pointer_cast<DataSignal>(m_signals[name]);
}
