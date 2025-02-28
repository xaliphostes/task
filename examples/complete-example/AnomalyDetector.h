#pragma once
#include <task/CachedTask.h>

// Task to detect anomalies in sensor data
class AnomalyDetector : public CachedTask {
  public:
    AnomalyDetector() : CachedTask(std::chrono::minutes(30)) {
        createDataSignal("anomaliesDetected");
    }

    void setData(const std::vector<SensorData> &data,
                 const std::string &sensorId) {
        m_data = data;
        m_sensorId = sensorId;

        // Generate cache key based on sensor ID and data size
        setCacheKey(sensorId + "_" + std::to_string(data.size()));
    }

  protected:
    void executeImpl() override {
        emitString("log", "Detecting anomalies for sensor " + m_sensorId);

        std::vector<ProcessedResult> anomalies;

        // Simple anomaly detection algorithm (z-score)
        for (const auto &metric : {"temperature", "humidity", "pressure"}) {
            // Calculate mean and standard deviation
            double sum = 0.0;
            int count = 0;

            for (const auto &entry : m_data) {
                auto it = entry.measurements.find(metric);
                if (it != entry.measurements.end()) {
                    sum += it->second;
                    count++;
                }
            }

            if (count == 0)
                continue;

            double mean = sum / count;

            double variance = 0.0;
            for (const auto &entry : m_data) {
                auto it = entry.measurements.find(metric);
                if (it != entry.measurements.end()) {
                    variance += (it->second - mean) * (it->second - mean);
                }
            }

            double stdDev = std::sqrt(variance / count);

            // Identify anomalies (z-score > 2.0)
            for (const auto &entry : m_data) {
                auto it = entry.measurements.find(metric);
                if (it != entry.measurements.end()) {
                    double zScore = std::abs(it->second - mean) / stdDev;

                    if (zScore > 2.0) {
                        ProcessedResult anomaly;
                        anomaly.sensorId = entry.sensorId;
                        anomaly.resultType = metric;
                        anomaly.value = it->second;
                        anomaly.timestamp = entry.timestamp;
                        anomaly.isAnomaly = true;

                        anomalies.push_back(anomaly);
                    }
                }
            }
        }

        // Report results
        ArgumentPack args;
        args.add<std::vector<ProcessedResult>>(anomalies);
        args.add<std::string>(m_sensorId);
        emit("anomaliesDetected", args);

        emitString("log", "Detected " + std::to_string(anomalies.size()) +
                              " anomalies for sensor " + m_sensorId);
    }

  private:
    std::vector<SensorData> m_data;
    std::string m_sensorId;
};
