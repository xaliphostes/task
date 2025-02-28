#pragma once
#include "types.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <task/Task.h>

namespace fs = std::filesystem;

// Reporter task for output
class ResultReporter : public Task {
  public:
    ResultReporter(const std::string &outputDir) : m_outputDir(outputDir) {
        // Ensure output directory exists
        fs::create_directories(outputDir);
    }

    void onAnomaliesDetected(const ArgumentPack &args) {
        const auto &anomalies = args.get<std::vector<ProcessedResult>>(0);
        const auto &sensorId = args.get<std::string>(1);

        if (anomalies.empty()) {
            emitString("log", "No anomalies detected for " + sensorId);
            return;
        }

        // Save anomalies to file
        std::string filename = m_outputDir + "/anomalies_" + sensorId + ".txt";
        std::ofstream file(filename);

        if (!file) {
            emitString("error", "Failed to open file for writing: " + filename);
            return;
        }

        file << "Anomalies detected for sensor " << sensorId << ":\n";
        file << "Timestamp,Metric,Value\n";

        for (const auto &anomaly : anomalies) {
            file << anomaly.timestamp << "," << anomaly.resultType << ","
                 << anomaly.value << "\n";
        }

        emitString("log", "Saved " + std::to_string(anomalies.size()) +
                              " anomalies to " + filename);
    }

    void onDataAggregated(const ArgumentPack &args) {
        const auto &results = args.get<std::vector<ProcessedResult>>(0);

        // Save aggregated results to file
        std::string filename = m_outputDir + "/aggregated_results.txt";
        std::ofstream file(filename, std::ios::app);

        if (!file) {
            emitString("error", "Failed to open file for writing: " + filename);
            return;
        }

        file << "Timestamp: "
             << std::chrono::system_clock::now().time_since_epoch().count()
             << "\n";
        file << "SensorID,Metric,Value\n";

        for (const auto &result : results) {
            file << result.sensorId << "," << result.resultType << ","
                 << result.value << "\n";
        }

        file << "\n";

        emitString("log", "Saved " + std::to_string(results.size()) +
                              " aggregated results to " + filename);
    }

  private:
    std::string m_outputDir;
};
