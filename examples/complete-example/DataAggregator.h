#pragma once
#include <task/Algorithm.h>
#include "types.h"

// Task for aggregating sensor data
class DataAggregator : public Algorithm {
  public:
    DataAggregator() { createDataSignal("aggregated"); }

    void exec(const ArgumentPack &args = {}) override {
        emitString("log", "Aggregating data for all sensors");

        std::map<std::string, std::map<std::string, std::vector<double>>>
            aggregatedData;

        // Aggregate metrics by sensor and measurement type
        for (const auto &[sensorId, data] : m_sensorData) {
            for (const auto &entry : data) {
                for (const auto &[metric, value] : entry.measurements) {
                    aggregatedData[sensorId][metric].push_back(value);
                }
            }
        }

        // Calculate statistics for each sensor and metric
        std::vector<ProcessedResult> results;
        int64_t currentTime =
            std::chrono::system_clock::now().time_since_epoch().count();

        for (const auto &[sensorId, metrics] : aggregatedData) {
            for (const auto &[metric, values] : metrics) {
                // Calculate min, max, mean
                double min = *std::min_element(values.begin(), values.end());
                double max = *std::max_element(values.begin(), values.end());

                double sum = std::accumulate(values.begin(), values.end(), 0.0);
                double mean = sum / values.size();

                // Create result entries
                ProcessedResult minResult{sensorId, metric + "_min", min,
                                          currentTime, false};
                ProcessedResult maxResult{sensorId, metric + "_max", max,
                                          currentTime, false};
                ProcessedResult avgResult{sensorId, metric + "_avg", mean,
                                          currentTime, false};

                results.push_back(minResult);
                results.push_back(maxResult);
                results.push_back(avgResult);
            }
        }

        // Report results
        ArgumentPack resultArgs;
        resultArgs.add<std::vector<ProcessedResult>>(results);
        emit("aggregated", resultArgs);

        emitString("log", "Aggregated " + std::to_string(results.size()) +
                              " metrics across " +
                              std::to_string(aggregatedData.size()) +
                              " sensors");
    }

    void addSensorData(const std::string &sensorId,
                       const std::vector<SensorData> &data) {
        m_sensorData[sensorId] = data;
        setDirty(true);
    }

  private:
    std::map<std::string, std::vector<SensorData>> m_sensorData;
};
