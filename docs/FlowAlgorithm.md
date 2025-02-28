[< Back](index.md)

# FlowAlgorithm

## Overview

The `FlowAlgorithm` class extends the `Algorithm` class to provide a framework for implementing algorithms that process a sequence of jobs. It serves as an intermediate layer in the algorithm hierarchy that adds job management capabilities while maintaining the execution control, signaling, and progress reporting features of the base `Algorithm` class.

## Features

- **Job Management**: Methods for adding and clearing jobs to be processed
- **Job Abstraction**: Support for arbitrary job types through `std::any`
- **Customizable Job Processing**: Abstract method for job-specific processing
- **Sequential or Parallel Execution**: Base for both sequential and parallel job processing algorithms
- **Signal-Based Communication**: Inherited signals for algorithm lifecycle events
- **Progress Reporting**: Built-in mechanism for reporting execution progress
- **Cancellation Support**: Ability to request and detect stop requests during job execution

## Class Interface

```cpp
class FlowAlgorithm : public Algorithm {
public:
    // Constructor & destructor
    FlowAlgorithm() = default;
    virtual ~FlowAlgorithm() = default;
    
    // Job management
    void addJob(const std::any &job);
    void clearJobs();
    
    // Core job processing method (must be implemented by subclasses)
    virtual void doJob(const std::any &job) = 0;
    
protected:
    // Job storage
    std::vector<std::any> m_jobs;
};
```

## Signals

FlowAlgorithm inherits and emits the following signals:

| Signal     | Type   | Description                                 | Arguments                   |
| ---------- | ------ | ------------------------------------------- | --------------------------- |
| `progress` | Data   | Reports execution progress                  | float (0.0 to 1.0)          |
| `started`  | Simple | Indicates algorithm has started execution   | None                        |
| `finished` | Simple | Indicates algorithm has completed execution | None                        |
| `error`    | Data   | Reports execution errors                    | std::string (error message) |
| `log`      | Data   | Reports job management operations           | std::string (log message)   |

## Usage Example

```cpp
// Create a custom flow algorithm by extending the FlowAlgorithm class
class TextProcessor : public FlowAlgorithm {
public:
    // Implement the required exec method to process all jobs
    void exec(const ArgumentPack& args = {}) override {
        if (m_jobs.empty()) {
            emitString("log", "No text files to process");
            return;
        }
        
        emit("started");
        
        size_t totalJobs = m_jobs.size();
        for (size_t i = 0; i < totalJobs; ++i) {
            // Check for cancellation
            if (stopRequested()) {
                emitString("warn", "Processing stopped by user");
                return;
            }
            
            // Process this job
            try {
                doJob(m_jobs[i]);
            } catch (const std::exception& e) {
                emitString("error", std::string("Error processing job: ") + e.what());
            }
            
            // Report progress
            float progress = static_cast<float>(i + 1) / totalJobs;
            reportProgress(progress);
        }
        
        emitString("log", "Text processing completed");
    }
    
    // Implement the required doJob method to process individual jobs
    void doJob(const std::any& job) override {
        try {
            // Try to cast the job to the expected type
            auto filename = std::any_cast<std::string>(job);
            
            // Process the text file
            emitString("log", "Processing file: " + filename);
            
            // File processing logic here...
            
            // Create result for this job
            ArgumentPack resultArgs;
            resultArgs.add<std::string>(filename);
            resultArgs.add<int>(wordCount);
            emit("file_processed", resultArgs);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid job type: expected string filename");
        }
    }
    
    // Add custom signal
    TextProcessor() {
        createDataSignal("file_processed");
    }
};

// Using the flow algorithm
void processTextFiles() {
    // Create the algorithm
    auto processor = std::make_shared<TextProcessor>();
    
    // Add jobs to process
    processor->addJob(std::string("file1.txt"));
    processor->addJob(std::string("file2.txt"));
    processor->addJob(std::string("file3.txt"));
    
    // Connect to signals
    processor->connectData("file_processed", [](const ArgumentPack& args) {
        std::string filename = args.get<std::string>(0);
        int wordCount = args.get<int>(1);
        
        std::cout << "Processed " << filename << ": " 
                  << wordCount << " words" << std::endl;
    });
    
    processor->connectData("progress", [](const ArgumentPack& args) {
        float progress = args.get<float>(0);
        std::cout << "Progress: " << (progress * 100) << "%" << std::endl;
    });
    
    // Execute asynchronously
    auto future = processor->run();
    
    // Wait for completion if needed
    future.wait();
}
```

## Lifecycle

A FlowAlgorithm goes through the following lifecycle:

1. **Construction**: Internal state initialized from the base Algorithm class
2. **Job Configuration**: Jobs are added via `addJob()` method
3. **Execution Start**: `run()` or `runImpl()` inherited from Algorithm is called, emits "started" signal
4. **Job Execution**: `exec()` implementation runs, processes each job via `doJob()`, may report progress
5. **Execution End**: Algorithm completes all jobs, emits "finished" signal
6. **Error Handling**: If exceptions occur, emits "error" signal with details

The job collection can be cleared and refilled for repeated executions.

## Job Management

The FlowAlgorithm class maintains a vector of `std::any` objects that represent the jobs to be processed:

- `addJob(const std::any& job)`: Adds a new job to the collection
- `clearJobs()`: Removes all jobs from the collection
- `doJob(const std::any& job)`: Abstract method that must be implemented by subclasses to process a single job

The use of `std::any` allows for flexibility in job types, but requires careful type checking in the `doJob()` implementation.

## Execution Strategy

FlowAlgorithm itself doesn't implement the `exec()` method, leaving it to subclasses to define how jobs are processed:

- **Sequential Processing**: Jobs can be processed one after another (e.g., in `SequentialAlgorithm`)
- **Parallel Processing**: Jobs can be processed concurrently (e.g., in `ParallelAlgorithm`)
- **Custom Strategies**: Subclasses can implement custom job execution strategies

## Best Practices

1. **Type Safety**: Use careful type checking in `doJob()` to handle the `std::any` job objects safely
2. **Job Granularity**: Design jobs to be self-contained units of work with appropriate granularity
3. **Progress Reporting**: Report progress based on job completion rather than sub-job steps
4. **State Management**: Consider job-specific state management if complex recovery is needed
5. **Error Handling**: Use try-catch blocks around individual job processing to prevent one job failure from stopping the entire algorithm
6. **Cancellation Support**: Check `stopRequested()` between jobs to allow for clean cancellation
7. **Standard Signals**: Use the inherited signals consistently for proper integration with other components

## Common Subclasses

FlowAlgorithm serves as the base for several specialized algorithm classes:

- **ParallelAlgorithm**: Processes jobs concurrently using multiple threads
- **SequentialAlgorithm**: Processes jobs one after another in sequence
- **PrioritizedAlgorithm**: Processes jobs according to a priority order
- **BatchAlgorithm**: Processes jobs in batches rather than individually

## Implementation Details

- The `m_jobs` container is not thread-safe by itself and should be protected in concurrent implementations
- The `addJob()` and `clearJobs()` methods emit log signals to report job management activities
- Job objects are stored by value via `std::any`, so large job data should be passed by reference or shared pointer
- Type checking and casting of job objects is the responsibility of the `doJob()` implementation