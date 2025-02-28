#pragma once
#include <string>
#include <map>

// Data structures for our application
struct SensorData {
    std::string sensorId;
    int64_t timestamp;
    std::map<std::string, double> measurements;
};

struct ProcessedResult {
    std::string sensorId;
    std::string resultType;
    double value;
    int64_t timestamp;
    bool isAnomaly;
};
