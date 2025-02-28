[< Back](index.md)

# For

## Overview

The `For` class implements a loop controller that represents an iteration process as a Task in the signaling framework. It provides iteration functionality similar to a standard for-loop (`for(start; stop; step)`) but with the ability to emit signals on each iteration and execute asynchronously. This allows other components to react to loop iterations and monitor the loop's progress.

## Features

- **Configurable Loop Parameters**: Customizable start, stop, and step values
- **Signal-Based Iteration**: Emits signals for each iteration step
- **Asynchronous Execution**: Support for running loops in background threads
- **Optional Parameters**: Flexible specification of loop parameters
- **Current Value Tracking**: Access to the current iteration value during execution
- **Loop Control**: Methods to configure and start the loop execution
- **Integration with Task Framework**: Inherits from the Task base class for signal infrastructure

## Class Interface

```cpp
class For : public Task {
public:
    // Constructor
    For(const ForParameters &params = ForParameters());
    
    // Loop configuration
    void set(const ForParameters &params = ForParameters());
    
    // Getters and setters for loop parameters
    int startValue() const;
    void setStartValue(int value);
    
    int stopValue() const;
    void setStopValue(int value);
    
    int stepValue() const;
    void setStepValue(int value);
    
    // Current iteration value
    int getCurrentValue() const;
    
    // Loop execution
    void start();
    std::future<void> startAsync();
    
private:
    int m_start;
    int m_stop;
    int m_step;
    int m_current;
};

// Helper structure for loop parameters
struct ForParameters {
    std::optional<int> start;
    std::optional<int> stop;
    std::optional<int> step;
    
    ForParameters() = default;
    ForParameters(int start_, int stop_, int step_ = 1);
};
```

## Signals

The For class emits the following signals:

| Signal | Type | Description                           | Arguments                                          |
| ------ | ---- | ------------------------------------- | -------------------------------------------------- |
| `tick` | Data | Emitted on each iteration of the loop | start (int), stop (int), current (int), step (int) |

## Usage Example

```cpp
// Create and use a For loop with signal handlers
void runLoop() {
    // Create a counter task
    auto counter = std::make_shared<Counter>();
    
    // Create a loop that iterates from 0 to 10 with step 2
    auto loop = std::make_shared<For>(ForParameters(0, 10, 2));
    
    // Connect the loop's tick signal to update the counter
    loop->connectData("tick", [counter](const ArgumentPack& args) {
        int current = args.get<int>(2); // Get current value (index 2)
        
        std::cout << "Loop iteration: " << current << std::endl;
        
        // Update counter based on current loop value
        counter->setValue(current);
    });
    
    // Connect counter's valueChanged signal
    counter->connectData("valueChanged", [](const ArgumentPack& args) {
        int oldValue = args.get<int>(0);
        int newValue = args.get<int>(1);
        
        std::cout << "Counter changed from " << oldValue 
                  << " to " << newValue << std::endl;
    });
    
    // Start the loop (synchronously)
    loop->start();
    
    // Or start asynchronously
    auto future = loop->startAsync();
    
    // Do other work while loop executes
    
    // Wait for completion if needed
    if (future.valid()) {
        future.wait();
    }
    
    std::cout << "Loop completed" << std::endl;
}

// Example with direct parameter configuration
void configureLoop() {
    auto loop = std::make_shared<For>();
    
    // Configure loop parameters directly
    loop->setStartValue(5);
    loop->setStopValue(25);
    loop->setStepValue(5);
    
    // Or use the set method with ForParameters
    loop->set(ForParameters(5, 25, 5));
    
    // Check current configuration
    std::cout << "Loop configured: " 
              << loop->startValue() << " to " 
              << loop->stopValue() << " by " 
              << loop->stepValue() << std::endl;
    
    // Start the loop
    loop->start();
}
```

## ForParameters Structure

The `ForParameters` structure provides a convenient way to specify loop parameters:

```cpp
// Create with explicit values
ForParameters params(0, 100, 2);

// Create with default values and set only specific parameters
ForParameters params;
params.start = 10;
params.stop = 50;
// step remains default (not set)

// Use optional parameters (only set what's needed)
ForParameters params;
params.stop = 100; // Only setting the stop value
```

The `std::optional` type allows specifying only the parameters that need to change, leaving others at their default values.

## Loop Execution

The For class provides two methods for executing the loop:

1. **Synchronous Execution**:
   ```cpp
   loop->start(); // Blocks until loop completes
   ```

2. **Asynchronous Execution**:
   ```cpp
   std::future<void> future = loop->startAsync(); // Returns immediately
   
   // Later, wait for completion
   future.wait();
   ```

During loop execution, the `tick` signal is emitted on each iteration with all loop state information.

## Tick Signal Arguments

The `tick` signal provides comprehensive information about the loop state:

| Index | Type | Description             |
| ----- | ---- | ----------------------- |
| 0     | int  | Start value of the loop |
| 1     | int  | Stop value of the loop  |
| 2     | int  | Current iteration value |
| 3     | int  | Step value of the loop  |

This allows signal handlers to access all aspects of the loop state.

## Parameter Validation

The For class performs basic validation of loop parameters:

```cpp
if (m_start > m_stop && m_step > 0) {
    emitString("warn", "Bad configuration of the ForLoop");
}
```

This warns users when the loop is misconfigured (e.g., infinite loops or loops that will never execute).

## Integration with Other Components

The For class can be integrated with other components in the task framework:

- **Counter**: Update a counter based on loop iterations
- **Algorithm**: Trigger algorithm execution at specific iterations
- **ProgressMonitor**: Track loop progress as a percentage
- **Logger**: Log loop execution details
- **ThreadPool**: Execute different work on each iteration

## Best Practices

1. **Configure Before Starting**: Set all loop parameters before calling start() or startAsync()
2. **Check for Misconfiguration**: Verify loop parameters are valid to avoid infinite loops
3. **Use Appropriate Step**: Match the step direction to the start/stop relationship (positive for ascending, negative for descending)
4. **Handle Signals Efficiently**: Keep tick signal handlers lightweight to avoid slowing the loop
5. **Consider Asynchronous Execution**: Use startAsync() for loops that shouldn't block the main thread
6. **Access Current Value**: Use getCurrentValue() only during loop execution, as it's undefined outside the loop's active state

## Implementation Details

- The loop executes with standard for-loop semantics: `for (m_current = m_start; m_current != m_stop; m_current += m_step)`
- The loop is exit condition is based on inequality (`!=`) rather than comparison (`<` or `>`)
- The current implementation doesn't have intrinsic pause or resume capabilities
- Signal emission happens within the loop body, so handlers execute synchronously unless running asynchronously
- The ForParameters struct uses std::optional to allow partial specification of parameters
- The step value defaults to 1 if not explicitly set