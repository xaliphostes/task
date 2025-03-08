#pragma once
#include <task/RetryableTask.h>

// Task for parsing sensor data files
class DataParser : public RetryableTask {
  public:
    DataParser() : RetryableTask(3, std::chrono::seconds(2)) {
        createSignal("parsed");
    }

    void setFile(const fs::path &filePath) { m_filePath = filePath; }

  protected:
    void runImpl() override {
        emitString("log", "Parsing file: " + m_filePath.string());

        // Simulate parsing a file with potential failure
        if (rand() % 10 == 0) {
            throw std::runtime_error("Simulated parsing error");
        }

        // Parse the file (simplified simulation)
        std::vector<SensorData> data;
        std::string sensorId = m_filePath.filename().string();
        sensorId = sensorId.substr(0, sensorId.find_first_of('.'));

        int64_t baseTimestamp =
            std::chrono::system_clock::now().time_since_epoch().count();

        // Generate some synthetic data
        for (int i = 0; i < 100; i++) {
            SensorData entry;
            entry.sensorId = sensorId;
            entry.timestamp =
                baseTimestamp - i * 1000000000; // 1 second intervals

            // Add some random measurements
            entry.measurements["temperature"] = 20.0 + (rand() % 100) / 10.0;
            entry.measurements["humidity"] = 40.0 + (rand() % 600) / 10.0;
            entry.measurements["pressure"] = 1010.0 + (rand() % 100) / 10.0;

            data.push_back(entry);
        }

        emit("parsed", ArgumentPack(data, m_filePath));

        emitString("log", "Successfully parsed " + std::to_string(data.size()) +
                              " data points from " + m_filePath.string());
    }

    void onRetry(int attempt, const std::exception &error) override {
        emitString("warn", "Retry attempt " + std::to_string(attempt) +
                               " for " + m_filePath.string() + ": " +
                               error.what());
    }

  private:
    fs::path m_filePath;
};
