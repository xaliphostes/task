[< Back](../index.md)

# The **dataframe** library integration with **task**
This example demonstrates how the two libraries can be integrated:

__The task library provides__:
- Concurrency framework with ThreadPool
- Progress tracking and reporting
- Signal-based communication between components
- Execution time measurement with Chronometer
- Logging infrastructure

__The dataframe library provides__:
- Data storage with Serie and Dataframe
- Functional data transformations with map, reduce, find
- Type-safe operations on collections

Together, they create a powerful data processing pipeline that can handle large datasets with parallel processing while providing good monitoring, error handling, and progress reporting.

1. Uses more of the **dataframe** library functions
   - Uses pipe operator (|) for chaining operations
   - Uses filter, map, and reduce functions
   - Uses zip to combine results

2. Takes advantage of the **task** library's features
   - ThreadPool for parallel processing
   - Chronometer for timing
   - Logger for centralized logging
   - Progress reporting
   - Cancellation support

3. Creates a clean integration between the libraries
   - **dataframe** library handles data processing and transformations
   - **task** library handles execution, monitoring, and communication

This demonstrates how these libraries can complement each other - the **dataframe** library providing functional data processing capabilities while the **task** library provides a robust execution framework with monitoring, parallelism, and communication.

```cpp
#include <task/ThreadPool.h>
#include <task/Logger.h>
#include <task/Chronometer.h>
#include <task/concurrent/Runnable.h>
#include <dataframe/Dataframe.h>
#include <dataframe/core/map.h>
#include <dataframe/core/reduce.h>
#include <dataframe/core/filter.h>
#include <dataframe/core/pipe.h>
#include <memory>
#include <iostream>

// Task that processes a Serie using df library functions
class SerieProcessor : public Runnable {
public:
    SerieProcessor(const std::string& name, const df::Serie<double>& data)
        : m_name(name), m_data(data) {}
    
    const df::Serie<double>& getResult() const { return m_result; }

protected:
    void runImpl() override {
        emitString("log", m_name + " started processing " + std::to_string(m_data.size()) + " values");
        
        // Process data using df library, taking advantage of pipe operator and functional operations
        m_result = m_data 
            | df::bind_filter([](double v, size_t) { return v > 0; })  // Keep positive values
            | df::bind_map([this](double v, size_t i) {
                // Report progress periodically
                if (i % 100 == 0) {
                    float progress = float(i) / m_data.size();
                    reportProgress(progress);
                    
                    // Check for cancellation
                    if (stopRequested()) {
                        throw std::runtime_error("Processing stopped");
                    }
                }
                return v * v;  // Square the values
            });
        
        // Calculate sum using reduce
        double sum = df::reduce([](double acc, double v, size_t) { return acc + v; }, m_result, 0.0);
        
        // Report results
        emit("result", ArgumentPack(n_name, sum,m_result.size()));

        emitString("log", m_name + " completed: processed " + 
                 std::to_string(m_result.size()) + " values, sum = " + 
                 std::to_string(sum));
    }

private:
    std::string m_name;
    df::Serie<double> m_data;
    df::Serie<double> m_result;
};

int main() {
    Logger      logger("DataProcessor");
    Chronometer chronometer;
    ThreadPool  pool;

    logger.connectAllSignalsTo(&chronometer);
    logger.connectAllSignalsTo(&pool);
    
    // Create sample data with df::range function
    auto data1 = df::Serie<double>({1.0, 2.0, 3.0, 4.0, 5.0, -1.0, -2.0});
    auto data2 = df::Serie<double>({10.0, 20.0, 30.0, -10.0, -20.0, 40.0, 50.0});
    
    // Add tasks to process each serie
    auto processor1 = pool.createAndAdd<SerieProcessor>("Data1", data1);
    auto processor2 = pool.createAndAdd<SerieProcessor>("Data2", data2);
    
    // Connect logger to processors
    logger.connectAllSignalsTo(processor1);
    logger.connectAllSignalsTo(processor2);
    
    // Connect to result signals
    processor1->connectData("result", [](const ArgumentPack& args) {
        std::string name = args.get<std::string>(0);
        double sum = args.get<double>(1);
        size_t count = args.get<size_t>(2);
        std::cout << name << " results: " << count << " values, sum = " << sum << std::endl;
    });
    
    processor2->connectData("result", [](const ArgumentPack& args) {
        std::string name = args.get<std::string>(0);
        double sum = args.get<double>(1);
        size_t count = args.get<size_t>(2);
        std::cout << name << " results: " << count << " values, sum = " << sum << std::endl;
    });
    
    // Start timing
    std::cout << "Starting parallel processing..." << std::endl;
    chronometer.start();
    
    // Execute all tasks in parallel
    pool.exec();
    
    // Stop timing
    int64_t elapsed = chronometer.stop();
    std::cout << "All processing completed in " << elapsed << " ms" << std::endl;
    
    // Perform additional operations like zip, merge, etc.
    auto combined = df::zip(processor1->getResult(), processor2->getResult());
    std::cout << "Combined " << combined.size() << " result pairs" << std::endl;
    
    return 0;
}
```