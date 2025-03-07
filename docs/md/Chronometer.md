[< Back](index.md)

# Chronometer

## Overview

The `Chronometer` class is a utility component in the task framework that provides precise time measurement capabilities. It extends the base `Task` class to integrate with the signal-slot system, allowing it to notify other components about timing events. The Chronometer provides a simple interface for measuring elapsed time between start and stop operations, making it ideal for performance profiling, operation timing, and benchmarking within the application.

## Features

- **High-Precision Timing**: Uses C++ standard library chrono for accurate time measurement
- **Signal-Based Notifications**: Emits signals for timing events and results
- **Simple Interface**: Easy-to-use start/stop API for time measurement
- **Millisecond Resolution**: Reports elapsed time in milliseconds
- **Error Reporting**: Provides feedback if timing operations are used incorrectly
- **Resource Management**: Automatically manages internal timing resources
- **Integration with Task Framework**: Works seamlessly with other Task-derived components

## Class Interface

```cpp
class Chronometer : public Task {
public:
    // Constructor
    Chronometer();

    // Start the timer
    void start();
    
    // Stop the timer and get elapsed time in milliseconds
    int64_t stop();

private:
    std::unique_ptr<std::chrono::system_clock::time_point> m_startTime;
};
```

## Signals

Chronometer emits the following signals:

| Signal     | Type   | Description                               | Arguments                            |
| ---------- | ------ | ----------------------------------------- | ------------------------------------ |
| `started`  | Simple | Emitted when the timer starts             | None                                 |
| `finished` | Simple | Emitted when the timer stops              | None                                 |
| `timing`   | Data   | Reports elapsed time on stop              | int64_t (elapsed time in ms)         |
| `error`    | Data   | Reports errors during timing operations   | std::string (error message)          |

## Usage Example

```cpp
// Basic timing example
void basicTiming() {
    // Create the chronometer
    auto timer = std::make_shared<Chronometer>();
    
    // Connect to signals
    timer->connectSimple("started", []() {
        std::cout << "Timer started" << std::endl;
    });
    
    timer->connectSimple("finished", []() {
        std::cout << "Timer stopped" << std::endl;
    });
    
    timer->connectData("timing", [](const ArgumentPack& args) {
        int64_t elapsedMs = args.get<int64_t>(0);
        std::cout << "Elapsed time: " << elapsedMs << " ms" << std::endl;
    });
    
    // Start the timer
    timer->start();
    
    // Perform the operation to be timed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Stop the timer and get the elapsed time
    int64_t elapsed = timer->stop();
    
    // Use the elapsed time
    if (elapsed > 1000) {
        std::cout << "Operation took more than 1 second!" << std::endl;
    }
}

// Using Chronometer to benchmark an algorithm
void benchmarkAlgorithm() {
    // Create algorithm and chronometer
    auto algorithm = std::make_shared<MyAlgorithm>();
    auto timer = std::make_shared<Chronometer>();
    
    // Connect chronometer to log timing info
    timer->connectData("timing", [](const ArgumentPack& args) {
        int64_t elapsedMs = args.get<int64_t>(0);
        std::cout << "Algorithm execution time: " << elapsedMs << " ms" << std::endl;
    });
    
    // Time the algorithm execution
    timer->start();
    algorithm->exec();
    timer->stop();
}
```

## Timing Implementation

The Chronometer uses the C++ standard library's chrono facilities for high-precision timing:

1. **Timer Start**: Creates a time point using `std::chrono::system_clock::now()`
2. **Timer Stop**: Calculates duration between start and current time
3. **Duration Calculation**: Converts duration to milliseconds using `std::chrono::duration_cast`

```cpp
void Chronometer::start() {
    m_startTime = std::make_unique<std::chrono::system_clock::time_point>(
        std::chrono::system_clock::now());
    emit("started");
}

int64_t Chronometer::stop() {
    if (!m_startTime) {
        emitString("error", "Chronometer not started.");
        return 0;
    }

    auto now = std::chrono::system_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - *m_startTime)
                        .count();

    // Create ArgumentPack with timing information
    ArgumentPack timingArgs;
    timingArgs.add<int64_t>(timeDiff);

    // Emit both signals
    emit("finished");
    emit("timing", timingArgs);

    m_startTime.reset();

    return timeDiff;
}
```

## Error Handling

The Chronometer includes basic error handling to prevent misuse:

- **Stop Without Start**: Calling `stop()` before `start()` emits an error signal
- **Double Start**: Calling `start()` multiple times overwrites the previous start time
- **Resource Management**: Uses smart pointers to avoid memory leaks

## Integration with Other Components

The Chronometer can be integrated with various components in the task framework:

- **Algorithm**: Time algorithm execution for performance analysis
- **ThreadPool**: Measure parallel execution time
- **Logger**: Log execution times for performance tracking
- **TaskQueue**: Profile task execution duration
- **Benchmark**: Create comprehensive benchmark suites with timing data

## Best Practices

1. **Timer Pairing**: Always pair `start()` with `stop()` calls
2. **Signal Usage**: Connect to the `timing` signal for automatic time logging
3. **Return Value Usage**: Use the return value of `stop()` when immediate timing data is needed
4. **Resource Cleanup**: Call `stop()` to ensure proper cleanup of timing resources
5. **Nested Timing**: Use multiple Chronometer instances for nested timing operations
6. **Error Handling**: Connect to the error signal to catch timing operation errors
7. **Timing Precision**: Be aware that system load can affect timing precision

## Performance Considerations

- **Overhead**: The Chronometer adds minimal overhead to the timed operation
- **Resolution**: The timing resolution is in milliseconds, suitable for most application timing needs
- **System Clock**: Uses system_clock which may be affected by system time changes
- **Signal Emission**: Signal handlers are executed synchronously and add to the measured time

## Implementation Details

- Uses `std::chrono::system_clock` for time measurement
- Stores the start time as a unique pointer to allow for null checking
- Timing resolution is in milliseconds (int64_t) for broad compatibility
- The `timing` signal is created during construction for consistency
- The start time is reset after `stop()` to prevent reuse without a new `start()` call
- Internal state is maintained by the m_startTime pointer being null or non-null