[< Back](index.md)

# ParallelAlgorithm

## Overview

The `ParallelAlgorithm` class extends the `FlowAlgorithm` class to provide a framework for executing multiple jobs concurrently. It implements the job execution strategy using C++ futures and asynchronous execution to process each job in a separate thread. This approach maximizes performance on multi-core systems while maintaining the signal-based communication and progress reporting features of the base classes.

## Features

- **Concurrent Job Execution**: Processes multiple jobs simultaneously using separate threads
- **Job-Level Progress Reporting**: Reports progress based on completed job count
- **Cancellation Support**: Allows stopping execution with proper cleanup of running jobs
- **Job-Specific Signals**: Emits signals when individual jobs start and finish
- **Comprehensive Error Handling**: Captures and reports exceptions from individual jobs
- **Flexible Job Types**: Supports arbitrary job types through the `std::any` mechanism
- **Thread Management**: Handles thread coordination and synchronization internally
- **Execution Control**: Inherits execution control features from the Algorithm base class

## Class Interface

```cpp
class ParallelAlgorithm : public FlowAlgorithm {
public:
    // Constructor & destructor
    ParallelAlgorithm();
    virtual ~ParallelAlgorithm() = default;
    
    // Execute all jobs in parallel
    void exec(const ArgumentPack &args = {}) override;
    
    // Inherited from FlowAlgorithm
    virtual void doJob(const std::any &job) = 0; // Must be implemented by subclasses
};
```

## Signals

ParallelAlgorithm emits the following signals:

| Signal         | Type   | Description                                | Arguments                          |
| -------------- | ------ | ------------------------------------------ | ---------------------------------- |
| `job_started`  | Data   | Emitted when a job starts processing       | job index (size_t)                 |
| `job_finished` | Data   | Emitted when a job completes               | job index (size_t), success (bool) |
| `progress`     | Data   | Reports overall execution progress         | float (0.0 to 1.0)                 |
| `started`      | Simple | Indicates algorithm has started execution  | None                               |
| `finished`     | Simple | Indicates algorithm has completed all jobs | None                               |
| `error`        | Data   | Reports errors during job execution        | std::string (error message)        |
| `log`          | Data   | Reports execution status                   | std::string (log message)          |
| `warn`         | Data   | Reports warnings during execution          | std::string (warning message)      |

## Usage Example

```cpp
// Create a custom parallel algorithm by extending the ParallelAlgorithm class
class ImageProcessor : public ParallelAlgorithm {
public:
    // Constructor registers additional signals if needed
    ImageProcessor() {
        createDataSignal("image_processed");
    }
    
    // Implementation of the required doJob method for processing a single job
    void doJob(const std::any& job) override {
        try {
            // Try to cast the job to the expected file path type
            std::string imagePath = std::any_cast<std::string>(job);
            
            // Process the image
            emitString("log", "Processing image: " + imagePath);
            
            // Simulate image processing
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Image processing logic would go here...
            ImageData result;
            result.width = 1024;
            result.height = 768;
            result.path = imagePath;
            
            // Report successful processing
            ArgumentPack resultArgs;
            resultArgs.add<std::string>(imagePath);
            resultArgs.add<ImageData>(result);
            emit("image_processed", resultArgs);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid job type: expected string path");
        } catch (const std::exception& e) {
            throw std::runtime_error("Error processing image: " + std::string(e.what()));
        }
    }
    
private:
    struct ImageData {
        std::string path;
        int width;
        int height;
    };
};

// Using the parallel algorithm
void processImages() {
    // Create the algorithm
    auto processor = std::make_shared<ImageProcessor>();
    
    // Add jobs to process
    processor->addJob(std::string("image1.jpg"));
    processor->addJob(std::string("image2.jpg"));
    processor->addJob(std::string("image3.jpg"));
    processor->addJob(std::string("image4.jpg"));
    processor->addJob(std::string("image5.jpg"));
    
    // Connect to signals
    processor->connectData("image_processed", [](const ArgumentPack& args) {
        std::string path = args.get<std::string>(0);
        std::cout << "Processed image: " << path << std::endl;
    });
    
    processor->connectData("progress", [](const ArgumentPack& args) {
        float progress = args.get<float>(0);
        std::cout << "Overall progress: " << (progress * 100) << "%" << std::endl;
    });
    
    processor->connectData("job_started", [](const ArgumentPack& args) {
        size_t jobIndex = args.get<size_t>(0);
        std::cout << "Starting job " << jobIndex << std::endl;
    });
    
    processor->connectData("job_finished", [](const ArgumentPack& args) {
        size_t jobIndex = args.get<size_t>(0);
        bool success = args.get<bool>(1);
        std::cout << "Job " << jobIndex << (success ? " completed successfully" : " failed") << std::endl;
    });
    
    processor->connectData("error", [](const ArgumentPack& args) {
        std::string error = args.get<std::string>(0);
        std::cerr << "ERROR: " << error << std::endl;
    });
    
    // Execute asynchronously
    auto future = processor->run();
    
    // Wait for completion if needed
    future.wait();
    
    std::cout << "All images processed" << std::endl;
}
```

## Parallel Execution Model

The ParallelAlgorithm implements a parallel execution model:

1. **Job Preparation**: Before execution starts, all jobs are prepared and validated
2. **Thread Creation**: A separate thread is created for each job using `std::async`
3. **Concurrent Execution**: All jobs execute simultaneously on different threads
4. **Progress Tracking**: Progress is reported as the percentage of completed jobs
5. **Result Collection**: The algorithm waits for all threads to complete or be cancelled
6. **Cleanup**: Resources are properly released even if execution is interrupted

The implementation uses `std::future<void>` to track job completion and handle exceptions.

## Job Processing

The ParallelAlgorithm executes each job in the following sequence:

1. **Pre-Execution Check**: Verify if execution should continue or has been stopped
2. **Job Start Signal**: Emit `job_started` signal with the job index
3. **Job Execution**: Call the `doJob()` method with the job data
4. **Job Completion Signal**: Emit `job_finished` signal with success status
5. **Progress Update**: Update and report overall progress based on completed job count

Each job is processed in its own thread, allowing for truly parallel execution.

## Cancellation Handling

The ParallelAlgorithm supports cancellation through the `stopRequested()` mechanism:

1. **Cancellation Request**: Client code calls `stop()` to request cancellation
2. **Cancellation Check**: Jobs check `stopRequested()` before starting
3. **Graceful Shutdown**: Already running jobs are allowed to complete
4. **Final Wait**: Algorithm waits for all running jobs to finish before returning

This allows for responsive cancellation while ensuring clean resource handling.

## Error Handling

Errors in individual jobs are handled carefully to prevent one job failure from affecting others:

1. **Exception Catching**: Each job thread catches and reports exceptions
2. **Error Signal**: Exceptions trigger an `error` signal with details
3. **Job Failure Signal**: The `job_finished` signal includes success status (false for errors)
4. **Continued Execution**: Other jobs continue executing despite individual failures
5. **Exception Propagation**: Job exceptions don't propagate to the calling thread

This approach allows robust error handling while maintaining parallel execution.

## Progress Reporting

Progress is calculated based on the number of completed jobs:

```cpp
float progress = static_cast<float>(i + 1) / static_cast<float>(m_jobs.size());
reportProgress(progress);
```

This provides a simple but effective way to monitor overall algorithm completion.

## Best Practices

1. **Resource Management**: Ensure resources are properly managed and released in the `doJob()` implementation
2. **Exception Safety**: Use RAII techniques in job implementations to ensure exception safety
3. **Job Independence**: Design jobs to be completely independent to maximize parallelism
4. **Load Balancing**: Try to create jobs of similar computational complexity for optimal performance
5. **Cancellation Support**: Implement cancellation checks in long-running jobs for better responsiveness
6. **Thread Safety**: Ensure job implementations are thread-safe when accessing shared resources
7. **Signal Handling**: Keep signal handlers lightweight to avoid blocking job threads
8. **Job Granularity**: Choose an appropriate job size that balances parallelism with overhead

## Integration with Other Components

ParallelAlgorithm integrates well with other components in the task framework:

- **ThreadPool**: For more controlled parallel execution with a fixed thread count
- **Logger**: For detailed logging of parallel operations
- **Chronometer**: For measuring execution time of parallel operations
- **ProgressMonitor**: For tracking overall progress across multiple parallel algorithms

## Implementation Details

- Uses `std::async` with `std::launch::async` to create true parallel execution
- Each job executes in its own thread, allowing for maximum CPU utilization
- The algorithm waits for all futures to complete, even if cancellation is requested
- Progress is reported in a thread-safe manner as jobs complete
- Signal emissions occur from worker threads, so handlers should be thread-safe
- No explicit thread pool is used, relying instead on the C++ runtime's thread management
- Job execution order is not guaranteed due to the parallel nature