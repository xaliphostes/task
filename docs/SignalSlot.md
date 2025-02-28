[< Back](index.md)

# SignalSlot

## Overview

The `SignalSlot` class implements a flexible event notification system that enables loosely coupled communication between components. It provides a mechanism for objects to register interest in events (signals) and receive notifications when those events occur, without requiring direct dependencies between the signal emitter and receiver. This implementation supports both parameterless signals and signals with arbitrary arguments.

## Features

- **Decoupled Communication**: Objects can communicate without direct dependencies
- **Two Signal Types**: Support for both simple (parameterless) signals and data signals (with arguments)
- **Type-Safe Arguments**: Strong typing for signal arguments via `ArgumentPack`
- **Dynamic Signal Creation**: Signals can be created at runtime by name
- **Method Binding**: Connect signals to instance methods or free functions
- **Connection Management**: Connections can be stored and disconnected when no longer needed
- **Resource Safety**: Automatic cleanup of disconnected slots
- **Flexible Argument Passing**: Support for multiple arguments of various types

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
    bool createSimpleSignal(const std::string &name);
    bool createDataSignal(const std::string &name);
    
    // Signal checking
    bool hasSignal(const std::string &name) const;
    SignalType getSignalType(const std::string &name) const;
    
    // Simple signal connections (no arguments)
    ConnectionHandle connectSimple(const std::string &name, std::function<void()> slot);
    template <typename T>
    ConnectionHandle connectSimple(const std::string &name, T *instance, void (T::*method)());
    
    // Data signal connections (with ArgumentPack)
    ConnectionHandle connectData(const std::string &name, std::function<void(const ArgumentPack &)> slot);
    template <typename T>
    ConnectionHandle connectData(const std::string &name, T *instance, void (T::*method)(const ArgumentPack &));
    
    // Signal emission
    void emit(const std::string &name); // For simple signals
    void emit(const std::string &name, const ArgumentPack &args); // For data signals
    void emitString(const std::string &name, const std::string &value); // Convenience for string argument
    
private:
    // Internal signal access methods
    std::shared_ptr<SimpleSignal> getSimpleSignal(const std::string &name);
    std::shared_ptr<DataSignal> getDataSignal(const std::string &name);
    
    // Signal storage
    std::map<std::string, std::shared_ptr<SignalBase>> m_signals;
    std::map<std::string, SignalType> m_signal_types;
    std::reference_wrapper<std::ostream> m_output_stream;
};
```

## Signal Types

The SignalSlot system supports two types of signals:

| Signal Type | Class          | Description                                     | Usage                                      |
| ----------- | -------------- | ----------------------------------------------- | ------------------------------------------ |
| `NO_ARGS`   | `SimpleSignal` | Signal with no arguments (simple notification)  | Status changes, triggers, completion events |
| `WITH_ARGS` | `DataSignal`   | Signal with arguments packed in `ArgumentPack` | Data updates, errors with details, results  |

## Usage Example

```cpp
// Class using the SignalSlot system
class FileDownloader : public SignalSlot {
public:
    FileDownloader() {
        // Create signals for various events
        createSimpleSignal("downloadStarted");
        createSimpleSignal("downloadCompleted");
        createDataSignal("progress");
        createDataSignal("error");
        createDataSignal("dataReceived");
    }
    
    void startDownload(const std::string& url) {
        // Signal that download is starting
        emit("downloadStarted");
        
        try {
            // Simulate download process
            for (int i = 0; i <= 10; ++i) {
                // Report progress
                ArgumentPack progressArgs;
                progressArgs.add<float>(i / 10.0f);
                progressArgs.add<std::string>(url);
                emit("progress", progressArgs);
                
                // Simulate getting data chunks
                if (i > 0 && i < 10) {
                    ArgumentPack dataArgs;
                    dataArgs.add<std::string>("Data chunk " + std::to_string(i));
                    dataArgs.add<size_t>(1024); // Chunk size
                    emit("dataReceived", dataArgs);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Signal completion
            emit("downloadCompleted");
        }
        catch (const std::exception& e) {
            // Signal error
            emitString("error", std::string("Download failed: ") + e.what());
        }
    }
};

// Using the SignalSlot system
void downloadExample() {
    // Create the downloader
    auto downloader = std::make_shared<FileDownloader>();
    
    // Connect to signals with lambda functions
    downloader->connectSimple("downloadStarted", []() {
        std::cout << "Download has started!" << std::endl;
    });
    
    downloader->connectSimple("downloadCompleted", []() {
        std::cout << "Download completed successfully!" << std::endl;
    });
    
    downloader->connectData("progress", [](const ArgumentPack& args) {
        float progress = args.get<float>(0);
        std::string url = args.get<std::string>(1);
        std::cout << "Downloading " << url << ": " 
                 << (progress * 100) << "%" << std::endl;
    });
    
    downloader->connectData("dataReceived", [](const ArgumentPack& args) {
        std::string data = args.get<std::string>(0);
        size_t size = args.get<size_t>(1);
        std::cout << "Received " << size << " bytes: " << data << std::endl;
    });
    
    downloader->connectData("error", [](const ArgumentPack& args) {
        std::string errorMsg = args.get<std::string>(0);
        std::cerr << "ERROR: " << errorMsg << std::endl;
    });
    
    // Start the download
    downloader->startDownload("https://example.com/file.zip");
}
```

## ConnectionHandle

The SignalSlot system provides a `ConnectionHandle` for managing signal-slot connections:

```cpp
// Get a connection handle when connecting to a signal
ConnectionHandle connection = object->connectData("dataReceived", myCallback);

// Later, disconnect when no longer needed
connection->disconnect();

// Check if still connected
if (connection->connected()) {
    // ...
}
```

Connection handles use shared ownership to ensure connections are properly managed even when handlers go out of scope.

## ArgumentPack

The `ArgumentPack` class provides a type-safe container for passing arbitrary arguments with signals:

```cpp
// Creating an ArgumentPack
ArgumentPack args;
args.add<std::string>("filename.txt");
args.add<int>(42);
args.add<double>(3.14159);

// Retrieving values by index and type
std::string filename = args.get<std::string>(0);
int count = args.get<int>(1);
double value = args.get<double>(2);

// Getting the number of arguments
size_t numArgs = args.size();

// Checking if empty
bool isEmpty = args.empty();

// Cloning an ArgumentPack (deep copy)
ArgumentPack copy = args.clone();
```

## Signal Creation and Connection

The SignalSlot system uses string names for signals, allowing for dynamic signal management:

```cpp
// Creating signals
createSimpleSignal("eventA");  // Simple notification
createDataSignal("eventB");    // With arguments

// Connecting to signals with free functions
connectSimple("eventA", []() { 
    std::cout << "Event A occurred" << std::endl; 
});

connectData("eventB", [](const ArgumentPack& args) {
    std::string msg = args.get<std::string>(0);
    std::cout << "Event B occurred: " << msg << std::endl;
});

// Connecting to signals with member functions
connectSimple("eventA", this, &MyClass::handleEventA);
connectData("eventB", this, &MyClass::handleEventB);
```

## Signal Emission

Signals are emitted by name, with appropriate arguments for data signals:

```cpp
// Emitting a simple signal
emit("eventA");

// Emitting a data signal
ArgumentPack args;
args.add<std::string>("Important message");
args.add<int>(42);
emit("eventB", args);

// Convenience method for string argument
emitString("eventB", "Simple message");
```

## Best Practices

1. **Signal Naming**: Use descriptive signal names that clearly indicate the event
2. **Error Handling**: Always handle exceptions when processing slots to prevent one handler from breaking others
3. **Connection Management**: Store connection handles for connections that need to be disconnected later
4. **Documentation**: Document the signals provided by a class, including their names and argument types
5. **Thread Safety**: Be aware that SignalSlot is not inherently thread-safe; add synchronization if needed
6. **Resource Management**: Disconnect signals when objects are destroyed to prevent dangling references
7. **Signal Granularity**: Create focused signals for specific events rather than overloading a single signal
8. **Consistency**: Maintain consistent argument order in ArgumentPack for each signal

## Inheritance and Extension

SignalSlot is designed to be inherited by classes that need to provide signal functionality:

```cpp
class MyComponent : public SignalSlot {
public:
    MyComponent() {
        // Create signals during construction
        createSimpleSignal("initialized");
        createDataSignal("stateChanged");
    }
    
    void initialize() {
        // Implementation...
        emit("initialized");
    }
    
    void setState(int newState) {
        int oldState = m_state;
        m_state = newState;
        
        // Notify about state change
        ArgumentPack args;
        args.add<int>(oldState);
        args.add<int>(newState);
        emit("stateChanged", args);
    }
    
private:
    int m_state = 0;
};
```

## Performance Considerations

- Signal emission has O(n) complexity where n is the number of connected slots
- Connections use weak pointers internally to allow automatic cleanup
- Disconnected slots are removed lazily during signal emission
- ArgumentPack uses type erasure which involves dynamic allocation for arguments
- String-based signal lookup adds a small overhead compared to direct signal objects

## Implementation Details

- Slots are wrapped in `std::function` objects for type erasure
- Signal implementations use vectors of weak pointers to connection objects
- Dead connections are automatically pruned during signal emission
- SignalSlot uses `std::cerr` by default for error reporting, but this can be changed
- ArgumentPack manages type information and storage using a type-erased container
- Connection handles use shared ownership to manage connection lifetime