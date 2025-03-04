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

#include <task/Counter.h>
#include "TEST.h"

// Helper class to catch signals from Counter
class CounterSignalCatcher : public SignalSlot {
  public:
    CounterSignalCatcher() { reset(); }

    void reset() {
        valueChangedCalled = false;
        limitReachedCalled = false;
        resetCalled = false;
        oldValue = 0;
        newValue = 0;
        isMinLimit = false;
        limitValue = 0;
        logMessages.clear();
        warnMessages.clear();
    }

    void onValueChanged(const ArgumentPack &args) {
        valueChangedCalled = true;
        if (args.size() >= 2) {
            oldValue = args.get<int>(0);
            newValue = args.get<int>(1);
        }
    }

    void onLimitReached(const ArgumentPack &args) {
        limitReachedCalled = true;
        if (args.size() >= 2) {
            isMinLimit = args.get<bool>(0);
            limitValue = args.get<int>(1);
        }
    }

    void onReset() { resetCalled = true; }

    void onLog(const ArgumentPack &args) {
        if (!args.empty()) {
            logMessages.push_back(args.get<std::string>(0));
        }
    }

    void onWarn(const ArgumentPack &args) {
        if (!args.empty()) {
            warnMessages.push_back(args.get<std::string>(0));
        }
    }

    bool valueChangedCalled;
    bool limitReachedCalled;
    bool resetCalled;
    int oldValue;
    int newValue;
    bool isMinLimit;
    int limitValue;
    std::vector<std::string> logMessages;
    std::vector<std::string> warnMessages;
};

TEST(Counter, Construction) {
    // Default construction
    Counter c1;
    EXPECT_EQ(c1.getValue(), 0);
    EXPECT_FALSE(c1.getMinValue().has_value());
    EXPECT_FALSE(c1.getMaxValue().has_value());

    // Construction with initial value
    Counter c2(10);
    EXPECT_EQ(c2.getValue(), 10);

    // Construction with min and max
    Counter c3(5, 0, 10);
    EXPECT_EQ(c3.getValue(), 5);
    EXPECT_EQ(*c3.getMinValue(), 0);
    EXPECT_EQ(*c3.getMaxValue(), 10);

    // Construction with invalid initial value (below min)
    CounterSignalCatcher catcher1;
    Counter c4(-5, 0, 10);
    c4.connect("warn", &catcher1, &CounterSignalCatcher::onWarn);

    // Should have adjusted value to minimum
    EXPECT_EQ(c4.getValue(), 0);

    // Construction with invalid initial value (above max)
    CounterSignalCatcher catcher2;
    Counter c5(15, 0, 10);
    c5.connect("warn", &catcher2, &CounterSignalCatcher::onWarn);

    // Should have adjusted value to maximum
    EXPECT_EQ(c5.getValue(), 10);
}

TEST(Counter, Signals) {
    Counter counter(5, 0, 10);
    CounterSignalCatcher catcher;

    // Connect signals
    counter.connect("valueChanged", &catcher,
                        &CounterSignalCatcher::onValueChanged);
    counter.connect("limitReached", &catcher,
                        &CounterSignalCatcher::onLimitReached);
    counter.connect("reset", &catcher, &CounterSignalCatcher::onReset);
    counter.connect("log", &catcher, &CounterSignalCatcher::onLog);
    counter.connect("warn", &catcher, &CounterSignalCatcher::onWarn);

    // Test valueChanged signal
    counter.setValue(7);
    EXPECT_TRUE(catcher.valueChangedCalled);
    EXPECT_EQ(catcher.oldValue, 5);
    EXPECT_EQ(catcher.newValue, 7);

    // Reset catcher
    catcher.reset();

    // Test limitReached signal (max)
    counter.setValue(10);
    EXPECT_TRUE(catcher.valueChangedCalled);
    EXPECT_TRUE(catcher.limitReachedCalled);
    EXPECT_FALSE(catcher.isMinLimit);
    EXPECT_EQ(catcher.limitValue, 10);

    // Reset catcher
    catcher.reset();

    // Test limitReached signal (min)
    counter.setValue(0);
    EXPECT_TRUE(catcher.valueChangedCalled);
    EXPECT_TRUE(catcher.limitReachedCalled);
    EXPECT_TRUE(catcher.isMinLimit);
    EXPECT_EQ(catcher.limitValue, 0);

    // Reset catcher
    catcher.reset();

    // Test reset signal
    counter.setValue(5);
    catcher.reset();
    counter.reset();
    EXPECT_TRUE(catcher.resetCalled);
    EXPECT_TRUE(catcher.valueChangedCalled);
    EXPECT_EQ(catcher.oldValue, 5);
    EXPECT_EQ(catcher.newValue, 5); // initial value was 5

    // Reset catcher
    catcher.reset();

    // Test warning signal
    counter.setValue(20);                     // out of range
    EXPECT_FALSE(catcher.valueChangedCalled); // Value shouldn't change
    EXPECT_TRUE(catcher.warnMessages.size() > 0);
    EXPECT_EQ(counter.getValue(), 5); // Value should remain at 5
}

TEST(Counter, BasicOperations) {
    Counter counter(5, 0, 10);

    // Test increment
    EXPECT_EQ(counter.increment(), 6);
    EXPECT_EQ(counter.getValue(), 6);

    // Test increment by specific amount
    EXPECT_EQ(counter.increment(3), 9);
    EXPECT_EQ(counter.getValue(), 9);

    // Test increment beyond max
    EXPECT_EQ(counter.increment(5), 10);
    EXPECT_EQ(counter.getValue(), 10);

    // Test decrement
    EXPECT_EQ(counter.decrement(), 9);
    EXPECT_EQ(counter.getValue(), 9);

    // Test decrement by specific amount
    EXPECT_EQ(counter.decrement(5), 4);
    EXPECT_EQ(counter.getValue(), 4);

    // Test decrement beyond min
    EXPECT_EQ(counter.decrement(10), 0);
    EXPECT_EQ(counter.getValue(), 0);

    // Test reset
    EXPECT_EQ(counter.reset(), 5);
    EXPECT_EQ(counter.getValue(), 5);

    // Test setValue
    EXPECT_TRUE(counter.setValue(8));
    EXPECT_EQ(counter.getValue(), 8);

    // Test invalid setValue
    EXPECT_FALSE(counter.setValue(15));
    EXPECT_EQ(counter.getValue(), 8); // Should remain unchanged
}

TEST(Counter, RangeLimits) {
    Counter counter(5);

    // Initially no limits
    EXPECT_FALSE(counter.isAtMinimum());
    EXPECT_FALSE(counter.isAtMaximum());

    // Set limits
    counter.setMinValue(0);
    counter.setMaxValue(10);

    // Test at min
    counter.setValue(0);
    EXPECT_TRUE(counter.isAtMinimum());
    EXPECT_FALSE(counter.isAtMaximum());

    // Test at max
    counter.setValue(10);
    EXPECT_FALSE(counter.isAtMinimum());
    EXPECT_TRUE(counter.isAtMaximum());

    // Test in middle
    counter.setValue(5);
    EXPECT_FALSE(counter.isAtMinimum());
    EXPECT_FALSE(counter.isAtMaximum());

    // Test changing limits
    counter.setValue(3);
    EXPECT_FALSE(counter.isAtMinimum());

    // Change min to current value
    counter.setMinValue(3);
    EXPECT_TRUE(counter.isAtMinimum());

    // Test invalid limit settings
    CounterSignalCatcher catcher;
    counter.connect("warn", &catcher, &CounterSignalCatcher::onWarn);

    // Try to set min > max
    EXPECT_FALSE(counter.setMinValue(15));
    EXPECT_TRUE(catcher.warnMessages.size() > 0);

    // Reset catcher
    catcher.reset();

    // Try to set max < min
    EXPECT_FALSE(counter.setMaxValue(2));
    EXPECT_TRUE(catcher.warnMessages.size() > 0);
}

TEST(Counter, AutoAdjustment) {
    Counter counter(5);
    CounterSignalCatcher catcher;

    counter.connect("valueChanged", &catcher,
                        &CounterSignalCatcher::onValueChanged);

    // Set max and verify value doesn't change
    counter.setMaxValue(10);
    EXPECT_FALSE(catcher.valueChangedCalled);
    EXPECT_EQ(counter.getValue(), 5);

    // Reset catcher
    catcher.reset();

    // Set max below current value and verify auto-adjustment
    counter.setMaxValue(4);
    EXPECT_TRUE(catcher.valueChangedCalled);
    EXPECT_EQ(counter.getValue(), 4);
    EXPECT_EQ(catcher.oldValue, 5);
    EXPECT_EQ(catcher.newValue, 4);

    // Reset catcher
    catcher.reset();

    // Set min and verify value doesn't change
    counter.setMinValue(1);
    EXPECT_FALSE(catcher.valueChangedCalled);
    EXPECT_EQ(counter.getValue(), 4);

    // Reset catcher
    catcher.reset();

    // Set min above current value and verify auto-adjustment
    counter.setMinValue(6);
    EXPECT_TRUE(catcher.valueChangedCalled);
    EXPECT_EQ(counter.getValue(), 6);
    EXPECT_EQ(catcher.oldValue, 4);
    EXPECT_EQ(catcher.newValue, 6);
}

TEST(Counter, RemovingLimits) {
    Counter counter(5, 0, 10);

    // Verify limits are set
    EXPECT_TRUE(counter.getMinValue().has_value());
    EXPECT_TRUE(counter.getMaxValue().has_value());

    // Remove min limit
    counter.setMinValue(std::nullopt);
    EXPECT_FALSE(counter.getMinValue().has_value());
    EXPECT_TRUE(counter.getMaxValue().has_value());

    // Now we should be able to go below the previous min
    counter.setValue(-5);
    EXPECT_EQ(counter.getValue(), -5);

    // Remove max limit
    counter.setMaxValue(std::nullopt);
    EXPECT_FALSE(counter.getMinValue().has_value());
    EXPECT_FALSE(counter.getMaxValue().has_value());

    // Now we should be able to go above the previous max
    counter.setValue(15);
    EXPECT_EQ(counter.getValue(), 15);
}

RUN_TESTS();