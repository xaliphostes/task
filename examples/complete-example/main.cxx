#include "SensorDataPipeline.h"
#include <filesystem>
#include <iostream>
#include <task/Chronometer.h>

namespace fs = std::filesystem;

int main() {
    // Directory paths
    std::string inputDir = "./sensor_data_input";
    std::string outputDir = "./sensor_data_output";

    try {
        // Create the pipeline
        SensorDataPipeline pipeline(inputDir, outputDir);

        // Create a chronometer to measure total execution time
        Chronometer chrono;

        // Create some test files
        std::cout << "Creating test sensor data files..." << std::endl;
        fs::create_directories(inputDir);

        for (int i = 1; i <= 5; i++) {
            std::string filename =
                inputDir + "/sensor" + std::to_string(i) + ".csv";
            std::ofstream file(filename);
            file << "timestamp,temperature,humidity,pressure\n";

            // Add some dummy data
            for (int j = 0; j < 100; j++) {
                file << time(nullptr) - j * 60 << ","
                     << 20.0 + (rand() % 100) / 10.0 << ","
                     << 40.0 + (rand() % 600) / 10.0 << ","
                     << 1010.0 + (rand() % 100) / 10.0 << "\n";
            }
        }

        // Start timing
        chrono.start();

        // Start the pipeline
        std::cout << "Starting the sensor data pipeline..." << std::endl;

        // Run in another thread so we can demonstrate stopping it later
        auto future = pipeline.runAsync();

        // Let it run for a while
        std::cout << "Pipeline running. Press Enter to stop..." << std::endl;
        std::cin.get();

        // Stop the pipeline
        pipeline.stop();

        // Wait for completion
        future.wait();

        // Stop timing
        int64_t elapsedTime = chrono.stop();

        std::cout << "Pipeline finished in " << elapsedTime << " ms"
                  << std::endl;
        std::cout << "Results can be found in " << outputDir << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
