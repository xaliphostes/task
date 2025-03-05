[< Back](index.md)

# If

## Overview

The `If` class provides a conditional control flow task in the task framework. It evaluates a condition and then executes either a "then" task or an "else" task based on the result. This enables building complex task workflows with conditional branching while maintaining the signal-based communication patterns of the framework.

## Features

- **Conditional Execution**: Execute different tasks based on runtime conditions
- **Fluent Interface**: Chainable methods for convenient configuration
- **Signal Propagation**: Forward signals from sub-tasks with branch context
- **Asynchronous Support**: Execute conditional logic in background threads
- **Flexible Conditions**: Support for condition functions that can access arguments
- **Execution Reporting**: Signals for condition evaluation and branch selection
- **Branch Monitoring**: Track which branch was executed

## Class Interface

```cpp
class If : public Task {
public:
    // Type definition for condition functions
    using ConditionFunction = std::function<bool(const ArgumentPack&)>;
    
    // Constructor
    explicit If(ConditionFunction condition);
    
    // Configuration methods (fluent interface)
    If& then(std::shared_ptr<Task> task);
    If& else_(std::shared_ptr<Task> task);
    
    // Execution methods
    void execute(const ArgumentPack& args = {});
    std::future<void> executeAsync(const ArgumentPack& args = {});
    
private:
    ConditionFunction m_condition;
    std::shared_ptr<Task> m_thenTask;
    std::shared_ptr<Task> m_elseTask;
};
```

## Signals

If emits the following signals:

| Signal                 | Type   | Description                               | Arguments                           |
| ---------------------- | ------ | ----------------------------------------- | ----------------------------------- |
| `started`              | Simple | Emitted when execution begins             | None                                |
| `finished`             | Simple | Emitted when execution completes          | None                                |
| `conditionEvaluated`   | Simple | Emitted after condition evaluation        | None                                |
| `branchSelected`       | Data   | Reports which branch was selected         | bool (result), string (branch name) |
| `thenExecuted`         | Simple | Emitted when the then branch completes    | None                                |
| `elseExecuted`         | Simple | Emitted when the else branch completes    | None                                |
| `noBranchExecuted`     | Simple | Emitted when no branch task was available | None                                |
| `log`, `warn`, `error` | Data   | Standard logging signals                  | string (message)                    |

## Usage Example

```cpp
// Creating a conditional task
auto checkValue = std::make_shared<If>([](const ArgumentPack& args) {
    // Condition that evaluates whether the first argument > 10
    if (args.empty()) return false;
    return args.get<int>(0) > 10;
});

// Configure the branches
auto highValueTask = std::make_shared<MyTask>("Process high value");
auto lowValueTask = std::make_shared<MyTask>("Process low value");

checkValue->then(highValueTask)
          ->else_(lowValueTask);

// Connect to signals
checkValue->connectData("branchSelected", [](const ArgumentPack& args) {
    bool result = args.get<bool>(0);
    std::string branch = args.get<std::string>(1);
    std::cout << "Selected branch: " << branch 
              << " (condition: " << (result ? "true" : "false") << ")" << std::endl;
});

// Create arguments
ArgumentPack args;
args.add<int>(15);  // This will cause the then branch to execute

// Execute the conditional task
checkValue->execute(args);
```

## Asynchronous Execution

The If task supports asynchronous execution:

```cpp
// Create arguments
ArgumentPack args;
args.add<int>(5);  // This will cause the else branch to execute

// Execute asynchronously
std::future<void> future = checkValue->executeAsync(args);

// Do other work while the conditional task executes

// Wait for completion when needed
future.wait();
```

## Condition Functions

The condition function determines which branch is executed:

```cpp
// Simple condition based on arguments
auto condition1 = [](const ArgumentPack& args) {
    return args.get<int>(0) > 10;
};

// Condition that captures external state
int threshold = 5;
auto condition2 = [threshold](const ArgumentPack& args) {
    return args.get<int>(0) > threshold;
};

// Condition that doesn't use arguments
auto condition3 = [](const ArgumentPack&) {
    return std::time(nullptr) % 2 == 0;  // Condition based on current time
};
```

## Signal Forwarding

The If task forwards signals from its branch tasks with appropriate context:

- Log messages from the then branch are prefixed with "then: "
- Log messages from the else branch are prefixed with "else: "
- Error signals from both branches are forwarded directly

## Combining with Other Task Components

The If task works well with other components in the task framework:

```cpp
// Create a thread pool
auto pool = std::make_shared<ThreadPool>();

// Create an If task
auto conditionalTask = std::make_shared<If>([](const ArgumentPack& args) {
    return args.get<bool>(0);
});

// Create branch tasks
auto thenBranch = std::make_shared<MyTask>("Then branch");
auto elseBranch = std::make_shared<MyTask>("Else branch");

// Configure the If task
conditionalTask->then(thenBranch)
               ->else_(elseBranch);

// Add the conditional task to the thread pool
pool->add(std::move(conditionalTask));

// Execute all tasks in the pool
pool->exec();
```

## Nesting Conditional Tasks

Conditional tasks can be nested to create complex decision trees:

```cpp
// Create outer condition
auto outerCondition = std::make_shared<If>([](const ArgumentPack& args) {
    return args.get<int>(0) > 0;
});

// Create inner condition (for positive numbers)
auto innerCondition = std::make_shared<If>([](const ArgumentPack& args) {
    return args.get<int>(0) > 10;
});

// Configure inner condition
auto highValueTask = std::make_shared<MyTask>("High value");
auto mediumValueTask = std::make_shared<MyTask>("Medium value");
innerCondition->then(highValueTask)
              ->else_(mediumValueTask);

// Configure outer condition
auto negativeValueTask = std::make_shared<MyTask>("Negative value");
outerCondition->then(innerCondition)  // Use inner condition as the then branch
              ->else_(negativeValueTask);

// Execute the nested conditions
ArgumentPack args;
args.add<int>(15);  // Will execute highValueTask
outerCondition->execute(args);
```

## Error Handling

The If task includes error handling to gracefully manage exceptions:

- Exceptions in the condition evaluation are caught and reported
- Exceptions in branch tasks are caught and forwarded
- If an exception occurs, the task still emits the finished signal

## Best Practices

1. **Keep Conditions Simple**: Focus on evaluation, avoid complex processing in condition functions
2. **Use Appropriate Task Types**: Choose the right type of task for each branch based on requirements
3. **Handle Branch Absence**: Be prepared for cases where a branch might not be defined
4. **Signal Connections**: Connect to the branchSelected signal for monitoring execution paths
5. **Error Handling**: Connect to error signals to detect and handle problems
6. **Resource Management**: Ensure branch tasks are properly managed (e.g., using shared_ptr)
7. **Argument Passing**: Design ArgumentPack contents to provide all necessary data for conditions and branches

## Implementation Details

- Condition functions are stored as std::function objects
- Branch tasks are stored as shared_ptr to ensure proper lifetime management
- Execute method handles both the condition evaluation and appropriate branch execution
- Signals provide detailed insight into the execution flow
- Branch selection is logged for debugging purposes
- Dynamic casting is used to accommodate different types of tasks as branches