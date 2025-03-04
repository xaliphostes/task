#include <chrono>
#include <iostream>
#include <task/Chronometer.h>
#include <task/For.h>
#include <task/Logger.h>
#include <task/ParallelAlgorithm.h>
#include <thread>

// Example job data structure
struct JobData {
    int id;
    std::string name;
    int processingTime; // in milliseconds
};

// Custom algorithm that implements parallel processing
class ProcessingAlgorithm : public ParallelAlgorithm {
  public:
    ProcessingAlgorithm() = default;

    // Implementation of the abstract method to process a job
    void doJob(const std::any &job) override {
        try {
            const auto &jobData = std::any_cast<const JobData &>(job);

            // Log job information
            emit("log",
                 ArgumentPack("Processing job #" + std::to_string(jobData.id) +
                              ": " + jobData.name));

            // Simulate processing time
            std::this_thread::sleep_for(
                std::chrono::milliseconds(jobData.processingTime));

            // Add some artificial progress reporting
            for (int i = 1; i <= 5; i++) {
                if (stopRequested()) {
                    emitString("warn", "Job #" + std::to_string(jobData.id) +
                                           " stopped mid-processing");
                    return;
                }

                float progress = static_cast<float>(i) / 5.0f;
                emit("progress", ArgumentPack(jobData.id, progress));

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(jobData.processingTime / 5));
            }

            // Log completion
            emit("log",
                 ArgumentPack("Completed job #" + std::to_string(jobData.id) +
                              ": " + jobData.name));
        } catch (const std::bad_any_cast &e) {
            emitString("error",
                       "Invalid job data type: " + std::string(e.what()));
            throw; // Re-throw to let ParallelAlgorithm handle it
        }
    }
};

int main() {
    // Create a logger
    Logger logger("[App]");

    // Create a chronometer for timing
    Chronometer chrono;

    // Setup chronometer timing reporting
    chrono.connect("timing", [](const ArgumentPack &args) {
        int64_t time = args.get<int64_t>(0);
        std::cout << "Operation took " << time << " ms" << std::endl;
    });

    // Create our processing algorithm
    ProcessingAlgorithm processor;

    // Connect logger to processor to capture all log messages
    logger.connectAllSignalsTo(&processor);

    // Connect signals for progress reporting
    processor.connect("progress", [](const ArgumentPack &args) {
        int jobId = args.get<int>(0);
        float progress = args.get<float>(1);
        std::cout << "Job #" << jobId << " progress: " << (progress * 100)
                  << "%" << std::endl;
    });

    // Connect to algorithm's started and finished signals
    processor.connect("started",
                      []() { std::cout << "Algorithm started" << std::endl; });

    processor.connect("finished",
                      []() { std::cout << "Algorithm finished" << std::endl; });

    // Connect chronometer to processor
    processor.connect("started", &chrono, &Chronometer::start);
    processor.connect("finished", [&chrono]() { chrono.stop(); });

    // Create and add some jobs
    for (int i = 1; i <= 5; i++) {
        JobData job{i, "Task-" + std::to_string(i),
                    200 * i}; // Varying processing times
        processor.addJob(job);
    }

    // Execute the algorithm
    auto future = processor.run();

    // Wait for completion (or could do other work here)
    future.wait();

    std::cout << "Main thread: Algorithm execution completed" << std::endl;

    return 0;
}
