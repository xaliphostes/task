Runnable and ThreadPool classes are part of a task management system with signal-slot capabilities.

These classes work together to create a thread pool implementation that can execute multiple runnable tasks in parallel:

## Runnable Class
The Runnable class is a base class for tasks that can be executed by the ThreadPool:
- It inherits from Task, which provides signal-slot functionality
- It defines a standard execution flow with proper signaling before and after execution
- It uses a Template Method pattern where concrete classes implement the runImpl() method

### Key features:
- Emits "started" and "finished" signals during execution
- Abstract runImpl() method must be implemented by subclasses
- Integrates with the signal-slot system from the parent Task class

## ThreadPool Class
The ThreadPool class manages parallel execution of multiple Runnable objects:
- It inherits from Algorithm, which provides additional functionality for execution control
- It stores and owns a collection of Runnable objects using std::unique_ptr
- It executes all runnables in parallel using std::async

### Key features:
- Can add runnables to the pool via add() method
- Has a template method createAndAdd<T>() to create and add a new runnable of type T
- Provides performance measurement when verbose mode is enabled
- Can detect the maximum available thread count via maxThreadCount()

These classes together provide a flexible framework for parallel task execution with proper signaling and resource management. The use of smart pointers ensures memory safety, and the signal-slot system allows for decoupled communication between components.
