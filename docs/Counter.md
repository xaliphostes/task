[< Back](index.md)

# Counter

## Overview

The `Counter` class is a versatile component in the task framework that provides a signal-emitting numeric counter with configurable bounds. It extends the base `Task` class to integrate with the signal-slot system, allowing other components to react to counter value changes and limit events. The Counter maintains an integer value that can be incremented, decremented, or directly set, with optional minimum and maximum bounds to constrain its range.

## Features

- **Value Management**: Methods to get, set, increment, and decrement the counter value
- **Bounded Range**: Optional minimum and maximum value constraints
- **Initial Value**: Configurable starting value with ability to reset to initial state
- **Limit Detection**: Methods to check if counter is at minimum or maximum values
- **Signal Notifications**: Emits signals on value changes and when limits are reached
- **Range Validation**: Automatically validates values against configured bounds
- **Automatic Value Adjustment**: Clamps values to stay within bounds when limits are applied
- **Integration with Task Framework**: Works seamlessly with other Task-derived components

## Class Interface

```cpp
class Counter : public Task {
public:
    // Constructor
    Counter(int initialValue = 0, std::optional<int> minValue = std::nullopt,
            std::optional<int> maxValue = std::nullopt);
    
    // Value access and modification
    int getValue() const;
    bool setValue(int value);
    int increment(int amount = 1);
    int decrement(int amount = 1);
    int reset();
    
    // Range checking
    bool isAtMinimum() const;
    bool isAtMaximum() const;
    
    // Bound management
    std::optional<int> getMinValue() const;
    std::optional<int> getMaxValue() const;
    bool setMinValue(std::optional<int> min);
    bool setMaxValue(std::optional<int> max);
    
private:
    int m_value;
    int m_initialValue;
    std::optional<int> m_minValue;
    std::optional<int> m_maxValue;
    
    bool isInRange(int value) const;
    void emitChangeSignals(int oldValue, int newValue);
};
```

## Signals

Counter emits the following signals:

| Signal          | Type   | Description                                 | Arguments                                      |
| --------------- | ------ | ------------------------------------------- | ---------------------------------------------- |
| `valueChanged`  | Data   | Emitted when the counter value changes      | oldValue (int), newValue (int)                 |
| `limitReached`  | Data   | Emitted when counter hits min or max limit  | isMinimum (bool), value (int)                  |
| `reset`         | Simple | Emitted when counter is reset               | None                                           |
| `log`           | Data   | Reports counter operations                  | std::string (log message)                      |
| `warn`          | Data   | Reports warnings (e.g., out-of-range)       | std::string (warning message)                  |

## Usage Example

```cpp
// Basic counter usage
void basicCounter() {
    // Create a counter with initial value 0, min 0, max 10
    auto counter = std::make_shared<Counter>(0, 0, 10);
    
    // Connect to signals
    counter->connectData("valueChanged", [](const ArgumentPack& args) {
        int oldValue = args.get<int>(0);
        int newValue = args.get<int>(1);
        std::cout << "Counter changed from " << oldValue 
                  << " to " << newValue << std::endl;
    });
    
    counter->connectData("limitReached", [](const ArgumentPack& args) {
        bool isMin = args.get<bool>(0);
        int value = args.get<int>(1);
        std::cout << "Counter reached " << (isMin ? "minimum" : "maximum") 
                  << " value: " << value << std::endl;
    });
    
    counter->connectSimple("reset", []() {
        std::cout << "Counter was reset" << std::endl;
    });
    
    // Use the counter
    counter->increment(5);     // 0 -> 5
    counter->increment(10);    // 5 -> 10 (max)
    std::cout << "Is at max: " << counter->isAtMaximum() << std::endl;
    counter->decrement(5);     // 10 -> 5
    counter->setValue(0);      // 5 -> 0 (min)
    std::cout << "Is at min: " << counter->isAtMinimum() << std::endl;
    counter->reset();          // Resets to initial value (0)
}

// Using Counter with a progress bar
void progressCounter() {
    // Create a counter for tracking progress (0-100)
    auto progress = std::make_shared<Counter>(0, 0, 100);
    auto progressBar = std::make_shared<ProgressBar>();  // Hypothetical component
    
    // Connect counter changes to update progress bar
    progress->connectData("valueChanged", [progressBar](const ArgumentPack& args) {
        int newValue = args.get<int>(1);
        progressBar->setProgress(newValue / 100.0f);
    });
    
    // Connect to limit signal to detect completion
    progress->connectData("limitReached", [](const ArgumentPack& args) {
        bool isMin = args.get<bool>(0);
        if (!isMin) {  // Max reached
            std::cout << "Process completed 100%" << std::endl;
        }
    });
    
    // Update progress as work is done
    for (int i = 0; i <= 10; i++) {
        // Do work...
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Update progress (0-100)
        progress->setValue(i * 10);
    }
}
```

## Range Constraints

The Counter supports optional minimum and maximum value constraints:

- **No Constraints**: When min and max are not set (`std::nullopt`), counter has unlimited range
- **Minimum Only**: Values below minimum are clamped to minimum
- **Maximum Only**: Values above maximum are clamped to maximum
- **Both Bounds**: Values are constrained to stay within [min, max] range

```cpp
// Counter with no constraints
auto unboundedCounter = std::make_shared<Counter>(0);

// Counter with only minimum value (>= 0)
auto nonNegativeCounter = std::make_shared<Counter>(0, 0, std::nullopt);

// Counter with only maximum value (<= 100)
auto limitedCounter = std::make_shared<Counter>(0, std::nullopt, 100);

// Counter with both min and max values (0-100)
auto boundedCounter = std::make_shared<Counter>(0, 0, 100);
```

When changing bounds, the counter ensures that min <= max and adjusts the current value if needed to stay within the new bounds.

## Value Validation

The Counter validates values against configured bounds:

- `setValue(value)`: Returns false if value is outside bounds (after warning)
- `increment(amount)`: Clamps result to maximum if it would exceed it
- `decrement(amount)`: Clamps result to minimum if it would go below it

The `isInRange(int value)` private method checks if a value is within the configured bounds.

## Signal Emission on Changes

The Counter emits signals when its value changes:

1. **valueChanged**: Emitted whenever the value changes, with old and new values
2. **limitReached**: Emitted when the counter reaches minimum or maximum limit
3. **reset**: Emitted when the counter is reset to its initial value

The `emitChangeSignals(int oldValue, int newValue)` private method handles all signal emissions related to value changes.

## Initial Value Management

The Counter maintains an initial value that can be reset to:

- **Constructor Setting**: The initial value is set during construction
- **Reset Method**: The `reset()` method returns the counter to its initial value
- **Bound Adjustment**: If initial value is outside bounds, it's adjusted and a warning is emitted

## Integration with Other Components

The Counter can be integrated with various components:

- **ProgressBar**: Use counter to drive a visual progress indicator
- **For**: Connect counter to a For loop to track iteration progress
- **Algorithm**: Use counter to track algorithm progress or iterations
- **ThreadPool**: Count active or completed threads
- **StateMachine**: Use counter for state transitions requiring numeric thresholds

## Best Practices

1. **Configure Bounds First**: Set min/max values before performing operations
2. **Check Return Values**: Verify return value of setValue() to ensure success
3. **Use Appropriate Methods**: Use increment/decrement for relative changes, setValue for absolute changes
4. **Signal Connections**: Connect to valueChanged for tracking changes, limitReached for completion detection
5. **Range Validation**: When an exact value is required, check return value of setValue()
6. **Initial Value**: Choose initial value within the intended range
7. **Reset for Reuse**: Use reset() to return to initial state for reuse

## Implementation Details

- Uses `std::optional<int>` for flexible bound specification
- Signal emissions include comprehensive information for flexible handling
- Bound validation ensures min <= max when both are specified
- The warning signal is used for non-fatal issues like out-of-range values
- Value clamping happens automatically during increment/decrement operations
- All value-changing operations emit appropriate signals for connected handlers
- The limitReached signal includes a boolean flag indicating whether minimum (true) or maximum (false) was reached