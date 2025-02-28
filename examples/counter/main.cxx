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

#include <iostream>
#include <task/Counter.h>
#include <task/Logger.h>

// An observer class that responds to counter events
class CounterObserver : public Task {
  public:
    CounterObserver(const std::string &name) : m_name(name) {}

    void onValueChanged(const ArgumentPack &args) {
        int oldValue = args.get<int>(0);
        int newValue = args.get<int>(1);

        std::cout << m_name << ": Value changed from " << oldValue << " to "
                  << newValue << std::endl;

        // Calculate difference
        int diff = newValue - oldValue;
        if (diff > 0) {
            std::cout << m_name << ": Increased by " << diff << std::endl;
        } else if (diff < 0) {
            std::cout << m_name << ": Decreased by " << -diff << std::endl;
        }
    }

    void onLimitReached(const ArgumentPack &args) {
        bool isMin = args.get<bool>(0);
        int value = args.get<int>(1);

        std::cout << m_name << ": Reached " << (isMin ? "minimum" : "maximum")
                  << " limit: " << value << std::endl;
    }

    void onReset() {
        std::cout << m_name << ": Counter was reset" << std::endl;
    }

  private:
    std::string m_name;
};

int main() {
    std::cout << "Counter Example" << std::endl;
    std::cout << "---------------" << std::endl;

    // Create a counter with range 0-10, starting at 5
    auto counter = std::make_shared<Counter>(5, 0, 10);

    // Create logger and observer
    auto logger = std::make_shared<Logger>("Counter:");
    auto observer = std::make_shared<CounterObserver>("Observer1");

    // Connect logger to counter
    logger->connectAllSignalsTo(counter.get());

    // Connect observer to specific counter signals
    counter->connectData("valueChanged", observer.get(),
                         &CounterObserver::onValueChanged);
    counter->connectData("limitReached", observer.get(),
                         &CounterObserver::onLimitReached);
    counter->connectSimple("reset", observer.get(), &CounterObserver::onReset);

    std::cout << "Initial value: " << counter->getValue() << std::endl;

    // Demonstrate increment
    std::cout << "\nIncrementing..." << std::endl;
    counter->increment();
    counter->increment(2);

    // Demonstrate reaching max limit
    std::cout << "\nApproaching maximum..." << std::endl;
    counter->setValue(9);
    counter->increment(); // Should hit max of 10
    counter->increment(); // Should remain at 10 with warning

    // Demonstrate decrement
    std::cout << "\nDecrementing..." << std::endl;
    counter->decrement();
    counter->decrement(3);

    // Demonstrate reaching min limit
    std::cout << "\nApproaching minimum..." << std::endl;
    counter->setValue(1);
    counter->decrement(); // Should hit min of 0
    counter->decrement(); // Should remain at 0 with warning

    // Demonstrate reset
    std::cout << "\nResetting..." << std::endl;
    counter->reset();

    // Demonstrate changing limits
    std::cout << "\nChanging limits..." << std::endl;
    std::cout << "Setting min to 2" << std::endl;
    counter->setMinValue(
        2); // This should also move the current value (5) up to 2

    std::cout << "Setting max to 8" << std::endl;
    counter->setMaxValue(8); // This shouldn't affect the current value

    // Demonstrate value clamping with the new limits
    std::cout << "\nTesting new limits..." << std::endl;
    counter->setValue(1); // Should fail and warn
    counter->setValue(9); // Should fail and warn
    counter->setValue(7); // Should succeed

    // Demonstrate removing limits
    std::cout << "\nRemoving limits..." << std::endl;
    counter->setMinValue(std::nullopt);
    counter->setMaxValue(std::nullopt);

    std::cout << "Testing without limits..." << std::endl;
    counter->setValue(-10); // Should now succeed
    counter->setValue(20);  // Should now succeed

    std::cout << "\nFinal value: " << counter->getValue() << std::endl;

    return 0;
}