[< Back](index.md)

# SignalSlot

## Overview

The `SignalSlot` class implements a thread-safe event notification system that enables loosely coupled communication between components. It provides a mechanism for objects to register interest in events (signals) and receive notifications when those events occur, without requiring direct dependencies between the signal emitter and receiver.

## Features

- **Thread-Safe Communication**: Multiple threads can safely connect to and emit signals
- **Unified Signal System**: Common interface for both parameterless and parameter-based signals
- **Type-Safe Arguments**: Strong typing for signal arguments via `ArgumentPack`
- **Synchronization Policies**: Control how signals are emitted across threads
- **Snapshot Emission**: Safe signal emission that avoids deadlocks and race conditions
- **Automatic Type Detection**: Identifies whether handlers require arguments at compile time
- **Connection Management**: Thread-safe connection creation and cleanup
- **Resource Safety**: Automatic cleanup of disconnected slots and proper thread synchronization
- **Flexible Argument Passing**: Support for multiple arguments with convenient constructor syntax

## Class Interface

```cpp
class SignalSlot {
public:
    // Constructor & destructor
    SignalSlot(std::ostream &output = std::cerr);
    virtual ~SignalSlot();
    
    // Output stream control
    void setOutputStream(std::ostream &stream);
    std::ostream &stream() const;
    
    // Signal creation
    bool createSignal(const std::string &name);
    
    // Signal checking
    bool hasSignal(const std::string &name) const;
    
    // Connect any callable to a signal
    template <typename Func>
    ConnectionHandle connect(const std::string &name, Func&& slot);
    
    // Connect member function to a signal
    template <typename T, typename Method>
    ConnectionHandle connect(const std::string &name, T* instance, Method method);
    
    // Signal emission
    void emit(const std::string &name, SyncPolicy policy = SyncPolicy::Direct);
    void emit(const std::string &name, const ArgumentPack &args, SyncPolicy policy = SyncPolicy::Direct);
    void emitString(const std::string &name, const std::string &value, SyncPolicy policy = SyncPolicy::Direct);
    
    // Cleanup
    void disconnectAllSignals();
};
```

## Synchronization Policies

The SignalSlot system offers different synchronization policies for signal emission:

```cpp
enum class SyncPolicy {
    Direct,    // Execute handlers directly in the emitting thread
    Blocking   // Block until all handlers have completed
};
```

These policies control how signal handlers are executed:

- **Direct**: The default policy. Handlers execute in the thread that emits the signal.
- **Blocking**: Ensures all handlers complete before the emission function returns.

## ArgumentPack

The `ArgumentPack` class provides a type-safe container for passing arbitrary arguments with signals:

```cpp
// Creating and populating an ArgumentPack (verbose way)
ArgumentPack args;
args.add<std::string>("filename.txt");
args.add<int>(42);
args.add<double>(3.14159);

// Creating ArgumentPack with variadic constructor (concise way)
ArgumentPack args("filename.txt", 42, 3.14159);

// Retrieving values by index and type
std::string filename = args.get<std::string>(0);
int count = args.get<int>(1);
double value = args.get<double>(2);
```

## Thread Safety Guarantees

The SignalSlot system provides the following thread safety guarantees:

- **Connection Safety**: Multiple threads can safely connect to signals concurrently
- **Emission Safety**: Multiple threads can safely emit signals concurrently
- **Snapshot Approach**: Signal emission takes a snapshot of active connections, allowing handlers to execute without holding locks
- **Handler Independence**: Handlers execute in the thread that emits the signal (by default)
- **Resource Protection**: All shared resources are protected by appropriate mutexes
- **No Handler Locks**: Locks are released before handler execution to avoid deadlocks

## Usage Examples

### Basic Signal Creation and Connection

```cpp
// Class that uses signals
class DataProcessor : public SignalSlot {
public:
    DataProcessor() {
        // Create signals for various events
        createSignal("started");
        createSignal("progress");
        createSignal("finished");
        createSignal("error");
    }
    
    void processData(const std::vector<double>& data) {
        emit("started");
        
        try {
            for (size_t i = 0; i < data.size(); ++i) {
                // Process data...
                double value = data[i] * 2;
                
                // Report progress (thread-safe)
                emit("progress", ArgumentPack(static_cast<float>(i + 1) / data.size(), value));
            }
            
            emit("finished");
        } catch (const std::exception& e) {
            emitString("error", e.what());
        }
    }
};
```

### Multi-threaded Usage

```cpp
// Shared component accessed from multiple threads
class SharedResource : public SignalSlot {
public:
    SharedResource() {
        createSignal("dataChanged");
        createSignal("stateChanged");
    }
    
    void updateData(const std::string& key, int value) {
        // Thread-safe signal emission
        emit("dataChanged", ArgumentPack(key, value));
    }
};

// Using the shared resource from multiple threads
void threadedExample() {
    auto resource = std::make_shared<SharedResource>();
    
    // Setup handlers
    resource->connect("dataChanged", [](const ArgumentPack& args) {
        std::string key = args.get<std::string>(0);
        int value = args.get<int>(1);
        std::cout << "Data changed: " << key << " = " << value << std::endl;
    });
    
    // Thread 1: Updates data
    std::thread t1([resource]() {
        for (int i = 0; i < 10; i++) {
            resource->updateData("counter", i);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Thread 2: Also updates data
    std::thread t2([resource]() {
        for (int i = 10; i < 20; i++) {
            resource->updateData("value", i * 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    });
    
    t1.join();
    t2.join();
}
```

### Using Synchronization Policies

```cpp
// Using different synchronization policies
void syncPolicyExample(SignalSlot& sender) {
    // Setup a signal
    sender.createSignal("event");
    
    // Connect a slow handler
    sender.connect("event", []() {
        std::cout << "Handler started" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Handler finished" << std::endl;
    });
    
    // Direct emission (returns immediately, handler runs in background)
    std::cout << "Direct emission..." << std::endl;
    sender.emit("event", SyncPolicy::Direct);
    std::cout << "Direct emission returned" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Blocking emission (waits for handler to complete)
    std::cout << "Blocking emission..." << std::endl;
    sender.emit("event", SyncPolicy::Blocking);
    std::cout << "Blocking emission returned" << std::endl;
}
```

## Best Practices for Thread-Safe Usage

1. **Handler Deadlock Prevention**: Avoid calling back into the emitting object from handlers, as this can cause deadlocks.
   ```cpp
   // Avoid this pattern
   void onSignal(const ArgumentPack& args) {
       // DON'T call methods that emit signals on the original emitter
       // as this could cause deadlocks or recursive locks
       emitter->doSomethingThatEmitsSignals();
   }
   ```

2. **Resource Locking**: Don't hold locks when connecting to or emitting signals.
   ```cpp
   void threadSafeMethod() {
       std::lock_guard<std::mutex> lock(m_mutex);
       
       // Bad: Holding a lock while emitting a signal
       emit("signal"); // Could lead to deadlocks
       
       // Better: Release the lock before emitting
   }
   
   void betterMethod() {
       // Do protected work
       std::string result;
       {
           std::lock_guard<std::mutex> lock(m_mutex);
           result = m_protectedResource;
       }
       
       // Now emit without holding the lock
       emitString("dataReady", result);
   }
   ```

3. **Thread Confinement**: For simplicity, consider confining signals and slots to specific threads.
   ```cpp
   // UI thread handler
   component->connect("uiUpdate", [this]() {
       // Only update UI from the UI thread
       if (isUiThread()) {
           updateUi();
       } else {
           // Queue for execution on UI thread
           queueOnUiThread([this]() { updateUi(); });
       }
   });
   ```

4. **Connection Management**: Store and manage connection handles for proper lifecycle management.
   ```cpp
   class Component {
   private:
       std::vector<ConnectionHandle> m_connections;
       
   public:
       void connect(SignalSlot* signaler) {
           m_connections.push_back(signaler->connect("signal", [this]() {
               handleSignal();
           }));
       }
       
       ~Component() {
           // Properly disconnect all connections
           for (auto& conn : m_connections) {
               if (conn && conn->connected()) {
                   conn->disconnect();
               }
           }
       }
   };
   ```

5. **Thread-Safe Handler Design**: Ensure handlers themselves are thread-safe.
   ```cpp
   // Thread-safe handler
   sharedObject->connect("dataChanged", [](const ArgumentPack& args) {
       static std::mutex handlerMutex;
       std::lock_guard<std::mutex> lock(handlerMutex);
       
       // Now safely access shared resources
       processData(args);
   });
   ```

## Advanced Techniques

### Thread-Safe Static Signals

Create static signals accessible from multiple threads:

```cpp
class GlobalEvents {
private:
    static std::mutex s_mutex;
    static std::map<std::string, std::shared_ptr<Signal>> s_signals;
    
public:
    static bool createSignal(const std::string& name) {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_signals.find(name) != s_signals.end())
            return false;
        s_signals[name] = std::make_shared<Signal>();
        return true;
    }
    
    template <typename Func>
    static ConnectionHandle connect(const std::string& name, Func&& slot) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_signals.find(name);
        if (it == s_signals.end())
            return nullptr;
        return it->second->connect(std::forward<Func>(slot));
    }
    
    static void emit(const std::string& name) {
        std::shared_ptr<Signal> signal;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto it = s_signals.find(name);
            if (it == s_signals.end())
                return;
            signal = it->second;
        }
        signal->emit();
    }
};
```

### Thread-Pool Handler Execution

For more control, implement a thread pool executor:

```cpp
class ThreadPoolExecutor {
private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop;
    
public:
    ThreadPoolExecutor(size_t numThreads) : m_stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            m_threads.emplace_back([this] {
                workerThread();
            });
        }
    }
    
    ~ThreadPoolExecutor() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_condition.notify_all();
        for (auto& thread : m_threads) {
            thread.join();
        }
    }
    
    void execute(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_condition.notify_one();
    }
    
private:
    void workerThread() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this] {
                    return m_stop || !m_tasks.empty();
                });
                
                if (m_stop && m_tasks.empty())
                    return;
                    
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            
            task();
        }
    }
};

// Usage with SignalSlot
class ThreadPoolSignalExecutor : public SignalSlot {
private:
    ThreadPoolExecutor m_executor;
    
public:
    ThreadPoolSignalExecutor(size_t numThreads) 
        : m_executor(numThreads) {}
    
    // Execute signal handlers in the thread pool
    void executeInThreadPool(const std::string& name) {
        auto signal = getSignal(name);
        if (!signal) return;
        
        m_executor.execute([signal]() {
            signal->emit();
        });
    }
};
```

## Performance Considerations

- **Mutex Overhead**: Thread safety adds some overhead due to mutex locking
- **Snapshot Approach**: Taking connection snapshots adds memory overhead but prevents deadlocks
- **Emission Cost**: Scales linearly with the number of connected slots
- **Lock Contention**: High-frequency signals from multiple threads can cause lock contention
- **Memory Usage**: Weak pointer usage helps manage memory but requires periodic cleanup

## Implementation Details

- **Thread Synchronization**: Uses mutexes and atomic variables for thread-safe state
- **Connection Snapshots**: Takes snapshots of active connections before calling handlers to avoid deadlocks
- **Lock-Free Execution**: Handlers execute without holding locks
- **Type Detection**: Uses template metaprogramming for compile-time handler type detection
- **Atomic Connection State**: Uses atomic variables for thread-safe connection state checks
- **Safe Disconnection**: Ensures proper cleanup of connections even during concurrent emissions