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

#pragma once
#include "Task.h"
#include <task/Counter.h>

/**
 * A simple counter class that uses the SignalSlot system to emit signals
 * when its value changes.
 */
class Counter : public Task {
  public:
    /**
     * Constructor for Counter
     * @param initialValue The initial value of the counter
     * @param minValue The minimum allowed value (default: no minimum)
     * @param maxValue The maximum allowed value (default: no maximum)
     */
    Counter(int initialValue = 0, std::optional<int> minValue = std::nullopt,
            std::optional<int> maxValue = std::nullopt);

    /**
     * Get the current value of the counter
     * @return The current count
     */
    int getValue() const;

    /**
     * Set the counter to a specific value
     * @param value The value to set
     * @return true if the value was set, false if it was out of bounds
     */
    bool setValue(int value);

    /**
     * Increment the counter by a specified amount
     * @param amount The amount to increment by (default: 1)
     * @return The new value after incrementing
     */
    int increment(int amount = 1);

    /**
     * Decrement the counter by a specified amount
     * @param amount The amount to decrement by (default: 1)
     * @return The new value after decrementing
     */
    int decrement(int amount = 1);

    /**
     * Reset the counter to its initial value
     * @return The value after reset
     */
    int reset();

    /**
     * Check if the counter is at its minimum value
     * @return true if at minimum, false otherwise
     */
    bool isAtMinimum() const;

    /**
     * Check if the counter is at its maximum value
     * @return true if at maximum, false otherwise
     */
    bool isAtMaximum() const;

    /**
     * Get the minimum value if set
     * @return The minimum value or nullopt if not set
     */
    std::optional<int> getMinValue() const;

    /**
     * Get the maximum value if set
     * @return The maximum value or nullopt if not set
     */
    std::optional<int> getMaxValue() const;

    /**
     * Set the minimum allowed value
     * @param min The new minimum value or nullopt to remove the limit
     * @return true if successful, false if min > max
     */
    bool setMinValue(std::optional<int> min);

    /**
     * Set the maximum allowed value
     * @param max The new maximum value or nullopt to remove the limit
     * @return true if successful, false if max < min
     */
    bool setMaxValue(std::optional<int> max);

  private:
    int m_value;
    int m_initialValue;
    std::optional<int> m_minValue;
    std::optional<int> m_maxValue;

    /**
     * Check if a value is within the allowed range
     * @param value The value to check
     * @return true if within range, false otherwise
     */
    bool isInRange(int value) const;

    /**
     * Emit appropriate signals for a value change
     * @param oldValue The previous value
     * @param newValue The new value
     */
    void emitChangeSignals(int oldValue, int newValue);
};