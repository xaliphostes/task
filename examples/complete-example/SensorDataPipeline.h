#pragma once
#include <task/CompositeAlgorithm.h>
#include <task/TaskGroup.h>
#include <task/TaskProfiler.h>
#include <task/TaskQueue.h>
#include <task/TaskSerializer.h>


// Main application orchestrator
class SensorDataPipeline : public CompositeAlgorithm {
  public:
    SensorDataPipeline(const std::string &inputDir,
                       const std::string &outputDir, int maxConcurrentTasks = 4)
        : m_inputDir(inputDir), m_outputDir(outputDir),
          m_maxConcurrentTasks(maxConcurrentTasks) {

        // Create directory if it doesn't exist
        fs::create_directories(inputDir);
        fs::create_directories(outputDir);

        // Initialize components
        initialize();
    }

    void initialize() {
        // Create logger
        m_logger = std::make_unique<Logger>("SensorPipeline");

        // Create task profiler
        m_profiler = std::make_unique<TaskProfiler>("profiler_output.json");

        // Create monitor for incoming files
        m_monitor = std::make_unique<DataMonitor>(m_inputDir);
        m_logger->connectAllSignalsTo(m_monitor.get());
        m_profiler->attachTo(m_monitor.get());

        // Create task queue for parsing
        m_parseQueue = std::make_unique<TaskQueue>(m_maxConcurrentTasks);
        m_logger->connectAllSignalsTo(m_parseQueue.get());

        // Create data aggregator
        m_aggregator = std::make_unique<DataAggregator>();
        m_logger->connectAllSignalsTo(m_aggregator.get());
        m_profiler->attachTo(m_aggregator.get());

        // Create task group for anomaly detection
        m_anomalyGroup = std::make_unique<TaskGroup>();
        m_logger->connectAllSignalsTo(m_anomalyGroup.get());

        // Create result reporter
        m_reporter = std::make_unique<ResultReporter>(m_outputDir);
        m_logger->connectAllSignalsTo(m_reporter.get());

        // Connect monitor to pipeline to handle new files
        m_monitor->connect("newFiles", [this](const ArgumentPack &args) {
            const auto &files = args.get<std::vector<fs::path>>(0);
            this->handleNewFiles(files);
        });

        // Setup persistent storage with TaskSerializer
        m_serializer = std::make_unique<TaskSerializer>(m_outputDir +
                                                        "/pipeline_state.json");
        m_serializer->registerTask("aggregator", m_aggregator.get());

        // Try to load previous state
        if (m_serializer->loadState()) {
            emitString("log", "Restored previous pipeline state");
        }

        // Setup periodic saving of state
        m_stateSaver = std::make_unique<PeriodicTask>(std::chrono::minutes(5));
        m_stateSaver->connect("execute", [this]() {
            this->m_serializer->saveState();
            this->emitString("log", "Saved pipeline state");
        });
    }

    void exec(const ArgumentPack &args = {}) override {
        emitString("log", "Starting sensor data pipeline");

        // Start the periodic state saver
        m_stateSaver->start();

        // Start file monitoring
        m_monitor->start();

        // Wait for stop signal
        while (!stopRequested()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Periodically check if there's data to aggregate
            if (m_pendingAggregation && !m_aggregator->isRunning() &&
                !m_anomalyGroup->isRunning()) {
                m_aggregator->run();
                m_pendingAggregation = false;
            }
        }

        // Stop all components
        m_monitor->requestStop();
        m_parseQueue->stopAll();
        m_anomalyGroup->stopAll();

        if (m_aggregator->isRunning()) {
            m_aggregator->stop();
        }

        m_stateSaver->requestStop();

        // Save final state
        m_serializer->saveState();

        emitString("log", "Sensor data pipeline stopped");
    }

  private:
    void handleNewFiles(const std::vector<fs::path> &files) {
        for (const auto &file : files) {
            // Create a parser task for this file
            auto parser = std::make_shared<DataParser>();
            m_logger->connectAllSignalsTo(parser.get());
            m_profiler->attachTo(parser.get());

            parser->setFile(file);

            // Connect parser to handle parsed data
            parser->connect("parsed", [this](const ArgumentPack &args) {
                const auto &data = args.get<std::vector<SensorData>>(0);
                const auto &file = args.get<fs::path>(1);
                this->handleParsedData(data, file);
            });

            // Add to the parse queue
            m_parseQueue->enqueue(parser, TaskPriority::NORMAL);
        }
    }

    void handleParsedData(const std::vector<SensorData> &data,
                          const fs::path &file) {
        if (data.empty()) {
            emitString("warn", "No data parsed from " + file.string());
            return;
        }

        // Extract sensor ID from first data point
        std::string sensorId = data[0].sensorId;

        // Add data to aggregator
        m_aggregator->addSensorData(sensorId, data);
        m_pendingAggregation = true;

        // Create anomaly detector for this sensor
        auto detector = std::make_shared<AnomalyDetector>();
        m_logger->connectAllSignalsTo(detector.get());
        m_profiler->attachTo(detector.get());

        detector->setData(data, sensorId);

        // Connect detector to reporter
        detector->connect("anomaliesDetected", m_reporter.get(),
                              &ResultReporter::onAnomaliesDetected);

        // Add to task group for execution
        m_anomalyGroup->addTask(detector);

        // Connect aggregator to reporter
        m_aggregator->connect("aggregated", m_reporter.get(),
                                  &ResultReporter::onDataAggregated);
    }

    std::string m_inputDir;
    std::string m_outputDir;
    int m_maxConcurrentTasks;
    bool m_pendingAggregation = false;

    std::unique_ptr<Logger> m_logger;
    std::unique_ptr<TaskProfiler> m_profiler;
    std::unique_ptr<DataMonitor> m_monitor;
    std::unique_ptr<TaskQueue> m_parseQueue;
    std::unique_ptr<DataAggregator> m_aggregator;
    std::unique_ptr<TaskGroup> m_anomalyGroup;
    std::unique_ptr<ResultReporter> m_reporter;
    std::unique_ptr<TaskSerializer> m_serializer;
    std::unique_ptr<PeriodicTask> m_stateSaver;
};
