/*
 * Copyright (c) 2024-now fmaerten@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include "TEST.h"
#include <string>
#include <task/FlowAlgorithm.h>
#include <vector>

// Concrete implementation of FlowAlgorithm for testing
class TestFlowAlgorithm : public FlowAlgorithm {
  public:
    TestFlowAlgorithm() : m_executed(false), m_totalProcessed(0) {}

    void exec(const ArgumentPack &args = {}) override {
        m_executed = true;
        m_processedJobs.clear();
        m_totalProcessed = 0;

        emit("started");

        for (const auto &job : m_jobs) {
            if (stopRequested()) {
                emit("warn", ArgumentPack("Execution stopped by request"));
                break;
            }

            doJob(job);

            // Update progress
            float progress =
                static_cast<float>(++m_totalProcessed) / m_jobs.size();
            reportProgress(progress);
        }

        emit("finished");
    }

    void doJob(const std::any &job) override {
        try {
            int value = std::any_cast<int>(job);
            m_processedJobs.push_back(value);

            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // Log the job processing
            ArgumentPack args;
            args.add<std::string>("Processed job with value: " +
                                  std::to_string(value));
            emit("log", args);
        } catch (const std::bad_any_cast &e) {
            emit("error",
                 ArgumentPack("Invalid job type: " + std::string(e.what())));
        }
    }

    bool wasExecuted() const { return m_executed; }
    const std::vector<int> &getProcessedJobs() const { return m_processedJobs; }
    size_t getTotalProcessed() const { return m_totalProcessed; }

  private:
    bool m_executed;
    std::vector<int> m_processedJobs;
    size_t m_totalProcessed;
};

// Helper to capture signals from FlowAlgorithm
class FlowSignalCatcher : public Task {
  public:
    FlowSignalCatcher() { reset(); }

    void reset() {
        m_started = false;
        m_finished = false;
        m_logMessages.clear();
        m_errorMessages.clear();
        m_warningMessages.clear();
        m_progressValues.clear();
    }

    void onStarted() { m_started = true; }

    void onFinished() { m_finished = true; }

    void onLog(const ArgumentPack &args) {
        if (!args.empty()) {
            m_logMessages.push_back(args.get<std::string>(0));
        }
    }

    void onError(const ArgumentPack &args) {
        if (!args.empty()) {
            m_errorMessages.push_back(args.get<std::string>(0));
        }
    }

    void onWarning(const ArgumentPack &args) {
        if (!args.empty()) {
            m_warningMessages.push_back(args.get<std::string>(0));
        }
    }

    void onProgress(const ArgumentPack &args) {
        if (!args.empty()) {
            m_progressValues.push_back(args.get<float>(0));
        }
    }

    bool started() const { return m_started; }
    bool finished() const { return m_finished; }
    const std::vector<std::string> &getLogMessages() const {
        return m_logMessages;
    }
    const std::vector<std::string> &getErrorMessages() const {
        return m_errorMessages;
    }
    const std::vector<std::string> &getWarningMessages() const {
        return m_warningMessages;
    }
    const std::vector<float> &getProgressValues() const {
        return m_progressValues;
    }

  private:
    bool m_started;
    bool m_finished;
    std::vector<std::string> m_logMessages;
    std::vector<std::string> m_errorMessages;
    std::vector<std::string> m_warningMessages;
    std::vector<float> m_progressValues;
};

// Basic test for constructor and inheritance
TEST(FlowAlgorithm, Constructor) {
    TestFlowAlgorithm flow;

    // Verify it's properly constructed
    EXPECT_FALSE(flow.wasExecuted());
    EXPECT_EQ(flow.getProcessedJobs().size(), 0);

    // Verify base class signals
    EXPECT_TRUE(flow.hasSignal("started"));
    EXPECT_TRUE(flow.hasSignal("finished"));
    EXPECT_TRUE(flow.hasSignal("log"));
    EXPECT_TRUE(flow.hasSignal("error"));
    EXPECT_TRUE(flow.hasSignal("warn"));
    EXPECT_TRUE(flow.hasSignal("progress"));
}

// Test job management
TEST(FlowAlgorithm, JobManagement) {
    TestFlowAlgorithm flow;

    // Add jobs
    flow.addJob(std::any(1));
    flow.addJob(std::any(2));
    flow.addJob(std::any(3));

    // Execute
    flow.exec();

    // Check jobs were processed
    EXPECT_TRUE(flow.wasExecuted());
    EXPECT_EQ(flow.getProcessedJobs().size(), 3);
    EXPECT_EQ(flow.getProcessedJobs()[0], 1);
    EXPECT_EQ(flow.getProcessedJobs()[1], 2);
    EXPECT_EQ(flow.getProcessedJobs()[2], 3);

    // Clear jobs and check
    flow.clearJobs();

    // Reset execution state for test
    TestFlowAlgorithm emptyFlow;
    emptyFlow.exec();

    EXPECT_TRUE(emptyFlow.wasExecuted());
    EXPECT_EQ(emptyFlow.getProcessedJobs().size(), 0);
}

// Test signal emissions
TEST(FlowAlgorithm, SignalEmissions) {
    TestFlowAlgorithm flow;
    FlowSignalCatcher catcher;

    // Connect signals
    flow.connect("started", &catcher, &FlowSignalCatcher::onStarted);
    flow.connect("finished", &catcher, &FlowSignalCatcher::onFinished);
    flow.connect("log", &catcher, &FlowSignalCatcher::onLog);
    flow.connect("error", &catcher, &FlowSignalCatcher::onError);
    flow.connect("warn", &catcher, &FlowSignalCatcher::onWarning);
    flow.connect("progress", &catcher, &FlowSignalCatcher::onProgress);

    // Add jobs
    flow.addJob(std::any(10));
    flow.addJob(std::any(20));
    flow.addJob(std::any(30));

    // Execute
    flow.exec();

    // Check signals
    EXPECT_TRUE(catcher.started());
    EXPECT_TRUE(catcher.finished());

    // Should have log messages for each job
    EXPECT_EQ(catcher.getLogMessages().size(), 3);
    EXPECT_TRUE(catcher.getLogMessages()[0].find("10") != std::string::npos);
    EXPECT_TRUE(catcher.getLogMessages()[1].find("20") != std::string::npos);
    EXPECT_TRUE(catcher.getLogMessages()[2].find("30") != std::string::npos);

    // Should have progress updates
    EXPECT_EQ(catcher.getProgressValues().size(), 3);
    EXPECT_NEAR(catcher.getProgressValues()[0], 1.0f / 3.0f, 0.01f);
    EXPECT_NEAR(catcher.getProgressValues()[1], 2.0f / 3.0f, 0.01f);
    EXPECT_NEAR(catcher.getProgressValues()[2], 3.0f / 3.0f, 0.01f);
}

// Test error handling with invalid job type
TEST(FlowAlgorithm, ErrorHandling) {
    TestFlowAlgorithm flow;
    FlowSignalCatcher catcher;

    // Connect error signal
    flow.connect("error", &catcher, &FlowSignalCatcher::onError);

    // Add jobs with mixed types
    flow.addJob(std::any(1));                           // Correct type
    flow.addJob(std::any(std::string("invalid type"))); // Wrong type
    flow.addJob(std::any(3));                           // Correct type

    // Execute
    flow.exec();

    // Check error was reported
    EXPECT_EQ(catcher.getErrorMessages().size(), 1);
    EXPECT_TRUE(catcher.getErrorMessages()[0].find("Invalid job type") !=
                std::string::npos);

    // Only valid jobs should be processed
    EXPECT_EQ(flow.getProcessedJobs().size(), 2);
    EXPECT_EQ(flow.getProcessedJobs()[0], 1);
    EXPECT_EQ(flow.getProcessedJobs()[1], 3);
}

// Test stop functionality
TEST(FlowAlgorithm, StopExecution) {
    TestFlowAlgorithm flow;
    FlowSignalCatcher catcher;

    // Connect signals
    flow.connect("warn", &catcher, &FlowSignalCatcher::onWarning);

    // Add many jobs
    for (int i = 0; i < 10; i++) {
        flow.addJob(std::any(i));
    }

    // Set up a thread to stop execution after a short delay
    std::thread stopThread([&flow]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        flow.stop();
    });

    // Execute
    flow.exec();

    // Wait for stop thread to complete
    stopThread.join();

    // Check that execution was stopped
    EXPECT_LT(flow.getTotalProcessed(), 10);
    EXPECT_TRUE(catcher.getWarningMessages().size() > 0);
    EXPECT_TRUE(catcher.getWarningMessages()[0].find("stopped") !=
                std::string::npos);
}

// Test async execution
TEST(FlowAlgorithm, AsyncExecution) {
    TestFlowAlgorithm flow;
    FlowSignalCatcher catcher;

    // Connect signals
    flow.connect("started", &catcher, &FlowSignalCatcher::onStarted);
    flow.connect("finished", &catcher, &FlowSignalCatcher::onFinished);

    // Add jobs
    for (int i = 0; i < 5; i++) {
        flow.addJob(std::any(i));
    }

    // Execute asynchronously
    auto future = flow.run();

    // Wait for completion
    future.wait();

    // Check results
    EXPECT_TRUE(catcher.started());
    EXPECT_TRUE(catcher.finished());
    EXPECT_EQ(flow.getTotalProcessed(), 5);
}

// Test dirty state
TEST(FlowAlgorithm, DirtyState) {
    TestFlowAlgorithm flow;

    // Add jobs
    flow.addJob(std::any(1));
    flow.addJob(std::any(2));

    // Initially dirty
    EXPECT_TRUE(flow.isDirty());

    // Execute - should clear dirty state
    flow.exec();
    EXPECT_FALSE(flow.isDirty());

    // Add more jobs - should set dirty again
    flow.addJob(std::any(3));
    EXPECT_TRUE(flow.isDirty());

    // Clear and check
    flow.clearJobs();
    EXPECT_TRUE(flow.isDirty());

    // Execute with no jobs
    flow.exec();
    EXPECT_FALSE(flow.isDirty());

    // Manual control of dirty state
    flow.setDirty(true);
    EXPECT_TRUE(flow.isDirty());
    flow.setDirty(false);
    EXPECT_FALSE(flow.isDirty());
}

RUN_TESTS();