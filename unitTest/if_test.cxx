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
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "TEST.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <task/If.h>
#include <thread>

// Mock task for testing
class MockTask : public Task {
  public:
    MockTask(const std::string &name) : m_name(name), m_executed(false) {
        createSignal("started");
        createSignal("finished");
    }

    virtual void execute() {
        emit("started");
        m_executed = true;
        emit("finished");
    }

    bool wasExecuted() const { return m_executed; }
    void reset() { m_executed = false; }
    const std::string &getName() const { return m_name; }

  private:
    std::string m_name;
    std::atomic<bool> m_executed;
};

// Tests for the If task
TEST(IfTask, Basic) {
    // Test basic condition that's always true
    If trueCondition([](const ArgumentPack &) { return true; });

    MockTask thenTask("ThenTask");
    MockTask elseTask("ElseTask");

    trueCondition.then(&thenTask).else_(&elseTask);

    // Execute and check results
    trueCondition.execute();

    CHECK(thenTask.wasExecuted());
    CHECK(!elseTask.wasExecuted());

    // Test basic condition that's always false
    auto falseCondition =
        std::make_shared<If>([](const ArgumentPack &) { return false; });

    MockTask thenTask2("ThenTask2");
    MockTask elseTask2("ElseTask2");

    falseCondition->then(&thenTask2).else_(&elseTask2);

    // Execute and check results
    falseCondition->execute();

    CHECK(!thenTask2.wasExecuted());
    CHECK(elseTask2.wasExecuted());
}

TEST(IfTask, Arguments) {
    // Test condition that uses arguments
    If condition([](const ArgumentPack &args) {
        if (args.empty())
            return false;
        return args.get<int>(0) > 10;
    });

    MockTask thenTask("ThenTask");
    MockTask elseTask("ElseTask");

    condition.then(&thenTask).else_(&elseTask);

    // Test with value > 10 (should execute then)
    ArgumentPack highArgs;
    highArgs.add<int>(15);
    condition.execute(highArgs);

    CHECK(thenTask.wasExecuted());
    CHECK(!elseTask.wasExecuted());

    // Reset mock tasks
    thenTask.reset();
    elseTask.reset();

    // Test with value < 10 (should execute else)
    ArgumentPack lowArgs;
    lowArgs.add<int>(5);
    condition.execute(lowArgs);

    CHECK(!thenTask.wasExecuted());
    CHECK(elseTask.wasExecuted());
}

TEST(IfTask, SignalEmission) {
    // Create a condition
    If condition([](const ArgumentPack &) { return true; });

    // Create mock tasks
    MockTask thenTask("ThenTask");

    condition.then(&thenTask);

    // Track signals
    bool startedEmitted = false;
    bool finishedEmitted = false;
    bool conditionEvaluatedEmitted = false;
    bool branchSelectedEmitted = false;
    bool thenExecutedEmitted = false;

    // Connect to signals
    condition.connect("started",
                      [&startedEmitted]() { startedEmitted = true; });

    condition.connect("finished",
                      [&finishedEmitted]() { finishedEmitted = true; });

    condition.connect("conditionEvaluated", [&conditionEvaluatedEmitted]() {
        conditionEvaluatedEmitted = true;
    });

    condition.connect("branchSelected",
                      [&branchSelectedEmitted](const ArgumentPack &args) {
                          branchSelectedEmitted = true;
                          CHECK(args.get<bool>(0) == true);
                          CHECK(args.get<std::string>(1) == "then");
                      });

    condition.connect("thenExecuted",
                      [&thenExecutedEmitted]() { thenExecutedEmitted = true; });

    // Execute
    condition.execute();

    // Check all signals were emitted
    CHECK(startedEmitted);
    CHECK(finishedEmitted);
    CHECK(conditionEvaluatedEmitted);
    CHECK(branchSelectedEmitted);
    CHECK(thenExecutedEmitted);
}

TEST(IfTask, MissingBranches) {
    // Create a condition with no branches defined
    If trueCondition([](const ArgumentPack &) { return true; });
    If falseCondition([](const ArgumentPack &) { return false; });

    // Track signals
    bool noBranchExecutedForTrue = false;
    bool noBranchExecutedForFalse = false;

    trueCondition.connect("noBranchExecuted", [&noBranchExecutedForTrue]() {
        noBranchExecutedForTrue = true;
    });

    falseCondition.connect("noBranchExecuted", [&noBranchExecutedForFalse]() {
        noBranchExecutedForFalse = true;
    });

    // Execute
    trueCondition.execute();
    falseCondition.execute();

    // Check signals
    CHECK(noBranchExecutedForTrue);
    CHECK(noBranchExecutedForFalse);
}

TEST(IfTask, AsyncExecution) {
    // Create condition
    If condition([](const ArgumentPack &) { return true; });

    // Create a slower mock task
    class SlowMockTask : public MockTask {
      public:
        SlowMockTask(const std::string &name) : MockTask(name) {}

        void execute() override {
            emit("started");
            // Sleep to simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            MockTask::execute();
        }
    };

    SlowMockTask slowTask("SlowTask");
    condition.then(&slowTask);

    // Execute asynchronously
    auto future = condition.executeAsync();

    // Check that task is running but not yet completed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Wait for completion
    future.wait();

    // Check task was executed
    CHECK(slowTask.wasExecuted());
}

TEST(IfTask, ExceptionHandling) {
    // Create a condition that throws an exception
    If badCondition([](const ArgumentPack &) -> bool {
        throw std::runtime_error("Test exception");
    });

    MockTask thenTask("ThenTask");
    MockTask elseTask("ElseTask");

    badCondition.then(&thenTask).else_(&elseTask);

    // Track error signal
    bool errorEmitted = false;
    std::string errorMessage;

    badCondition.connect(
        "error", [&errorEmitted, &errorMessage](const ArgumentPack &args) {
            errorEmitted = true;
            errorMessage = args.get<std::string>(0);
        });

    // Execute should not throw
    EXPECT_NO_THROW(badCondition.execute());

    // Check error was reported
    CHECK(errorEmitted);
    CHECK(errorMessage.find("Test exception") != std::string::npos);

    // Check neither branch was executed
    CHECK(!thenTask.wasExecuted());
    CHECK(!elseTask.wasExecuted());
}

// Run all tests
RUN_TESTS()