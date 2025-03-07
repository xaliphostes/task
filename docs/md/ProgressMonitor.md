[< Back](index.md)

# ProgressMonitor

## Overview

The `ProgressMonitor` class is a specialized task that tracks and responds to progress events across multiple tasks. It provides a central mechanism for monitoring task execution progress, consolidating progress information, and generating notifications when significant milestones are reached.

## Features

- **Centralized Progress Tracking**: Monitors progress across multiple tasks from a single point
- **Milestone Notifications**: Emits signals when significant progress thresholds are reached
- **Task Execution Monitoring**: Tracks task starts, completions, and failures
- **Aggregated Progress Calculation**: Computes overall progress across multiple tasks
- **Custom Milestone Definition**: Configurable progress thresholds for notifications
- **Summary Generation**: Produces consolidated execution reports and statistics

## Class Interface

```cpp
class ProgressMonitor : public Task {
public:
    // Constructor
    ProgressMonitor(float milestoneStep = 0.25f);
    
    // Event handlers
    void onProgress(const ArgumentPack& args);
    void onTaskStarted();
    void onTaskFinished();
    void onTaskError(const ArgumentPack& args);
    
    // Configuration
    void setTaskCount(int count);
    void setMilestoneStep(float step);
    void resetProgress();
    
    // Status methods
    float getOverallProgress() const;
    int getStartedTaskCount() const;
    int getCompletedTaskCount() const;
    int getFailedTaskCount() const;
    bool isComplete() const;
    
    // Milestone management
    void addCustomMilestone(float milestone, const std::string& name);
    void clearCustomMilestones();
};
```

## Signals

ProgressMonitor emits the following signals:

| Signal             | Description                          | Arguments                                               |
| ------------------ | ------------------------------------ | ------------------------------------------------------- |
| `progressUpdated`  | When overall progress changes        | overallProgress, completedTasks, totalTasks             |
| `milestoneReached` | When a progress milestone is reached | milestoneName, milestoneValue, overallProgress          |
| `allTasksComplete` | When all tasks have completed        | totalTasks, successCount, failureCount, totalTimeMs     |
| `summary`          | When a progress summary is generated | taskCount, completedCount, failedCount, overallProgress |

## Usage Example

```cpp
// Create a progress monitor
auto progressMonitor = std::make_shared<ProgressMonitor>();

// Set the total number of tasks to monitor
progressMonitor->setTaskCount(5);

// Connect to milestone signals
progressMonitor->connectData("milestoneReached", [](const ArgumentPack& args) {
    std::string milestoneName = args.get<std::string>(0);
    float progress = args.get<float>(2);
    
    std::cout << "Milestone reached: " << milestoneName 
              << " (" << (progress * 100) << "%)" << std::endl;
});

// Connect to completion signal
progressMonitor->connectData("allTasksComplete", [](const ArgumentPack& args) {
    int totalTasks = args.get<int>(0);
    int successCount = args.get<int>(1);
    int64_t totalTimeMs = args.get<int64_t>(3);
    
    std::cout << "All tasks completed: " << successCount << "/" << totalTasks
              << " succeeded in " << totalTimeMs << "ms" << std::endl;
});

// Connect tasks to the monitor
for (auto& task : tasks) {
    // Connect task signals to monitor handlers
    task->connectSimple("started", progressMonitor.get(), &ProgressMonitor::onTaskStarted);
    task->connectSimple("finished", progressMonitor.get(), &ProgressMonitor::onTaskFinished);
    task->connectData("error", progressMonitor.get(), &ProgressMonitor::onTaskError);
    task->connectData("progress", progressMonitor.get(), &ProgressMonitor::onProgress);
}

// Add custom milestones if needed
progressMonitor->addCustomMilestone(0.5f, "Halfway Point");
progressMonitor->addCustomMilestone(0.9f, "Almost Complete");

// Get current progress information
float currentProgress = progressMonitor->getOverallProgress();
int completedTasks = progressMonitor->getCompletedTaskCount();
```

## How It Works

The ProgressMonitor operates through the following mechanisms:

1. **Task Registration**: 
   - Set the total number of tasks expected with `setTaskCount()`
   - Connect task signals to monitor handler methods

2. **Progress Tracking**:
   - When a task reports progress, `onProgress()` updates its tracking
   - Overall progress is calculated based on individual task progress
   - Aggregated progress is calculated as an average of all task progress

3. **Milestone Detection**:
   - Standard milestones are tracked at configurable intervals (default: 25%, 50%, 75%, 100%)
   - Custom milestones can be added for application-specific thresholds
   - When progress crosses a milestone, the `milestoneReached` signal is emitted

4. **Task Lifecycle Monitoring**:
   - `onTaskStarted()` tracks when tasks begin execution
   - `onTaskFinished()` records successful completions
   - `onTaskError()` records failed tasks

5. **Completion Detection**:
   - When all expected tasks have completed (success or failure)
   - The `allTasksComplete` signal is emitted with summary information

## Progress Calculation Methods

ProgressMonitor supports multiple methods for calculating overall progress:

1. **Average Progress** (default): The average of all task progress values
2. **Weighted Progress**: Tasks can be assigned different weights based on complexity
3. **Completion Counting**: Progress based on the proportion of completed tasks
4. **Custom Calculation**: Application-specific progress formulas

The calculation method can be configured based on application needs.

## Use Cases

ProgressMonitor is particularly useful for:

1. **Complex Operations**: Tracking progress across multi-stage operations
2. **UI Progress Bars**: Providing accurate progress information to users
3. **Batch Processing**: Monitoring and reporting on batch job progress
4. **ETL Workflows**: Tracking data extraction, transformation, and loading progress
5. **Installation/Update Processes**: Monitoring multi-step installation progress

## Implementation Details

- Uses a map to track individual task progress
- Calculates weighted progress based on task complexity if configured
- Detects and reports milestone crossings only once
- Maintains accurate statistics even when tasks fail
- Thread-safe implementation with mutex protection where needed
- Optimized for minimal overhead on task execution

## Best Practices

1. **Set Task Count Early**: Initialize with the correct number of expected tasks
2. **Connect All Signals**: Ensure all tasks connect their progress and lifecycle signals
3. **Custom Milestones**: Add application-specific milestones based on user expectations
4. **Handle Task Failures**: Use error handling to ensure progress reporting is accurate even when tasks fail
5. **Reset Between Operations**: Call `resetProgress()` when starting a new batch of tasks