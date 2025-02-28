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

#include <algorithm>
#include <sstream>
#include <task/Counter.h>

Counter::Counter(int initialValue, std::optional<int> minValue,
                 std::optional<int> maxValue)
    : m_value(initialValue), m_initialValue(initialValue), m_minValue(minValue),
      m_maxValue(maxValue) {
    // Create standard signals
    createDataSignal("valueChanged"); // Emitted when the value changes
    createDataSignal("limitReached"); // Emitted when hitting min or max
    createSimpleSignal("reset");      // Emitted when reset is called

    // Check initial value is within range
    if (!isInRange(m_value)) {
        ArgumentPack args;
        std::stringstream ss;
        ss << "Initial value " << m_value << " is outside allowed range";
        if (m_minValue.has_value()) {
            ss << " (min: " << *m_minValue;
        }
        if (m_maxValue.has_value()) {
            ss << ", max: " << *m_maxValue;
        }
        ss << ")";
        args.add<std::string>(ss.str());
        emit("warn", args);

        // Adjust to be within range
        if (m_minValue.has_value() && m_value < *m_minValue) {
            m_value = *m_minValue;
        } else if (m_maxValue.has_value() && m_value > *m_maxValue) {
            m_value = *m_maxValue;
        }

        // Update initial value to the corrected value
        m_initialValue = m_value;
    }
}

int Counter::getValue() const { return m_value; }

bool Counter::setValue(int value) {
    if (!isInRange(value)) {
        ArgumentPack args;
        std::stringstream ss;
        ss << "Value " << value << " is outside allowed range";
        if (m_minValue.has_value()) {
            ss << " (min: " << *m_minValue;
        }
        if (m_maxValue.has_value()) {
            ss << ", max: " << *m_maxValue;
        }
        ss << ")";
        args.add<std::string>(ss.str());
        emit("warn", args);
        return false;
    }

    int oldValue = m_value;
    m_value = value;

    emitChangeSignals(oldValue, m_value);
    return true;
}

int Counter::increment(int amount) {
    int oldValue = m_value;
    int newValue = m_value + amount;

    // Clamp to range if needed
    if (m_maxValue.has_value() && newValue > *m_maxValue) {
        newValue = *m_maxValue;
    }

    m_value = newValue;
    emitChangeSignals(oldValue, m_value);
    return m_value;
}

int Counter::decrement(int amount) {
    int oldValue = m_value;
    int newValue = m_value - amount;

    // Clamp to range if needed
    if (m_minValue.has_value() && newValue < *m_minValue) {
        newValue = *m_minValue;
    }

    m_value = newValue;
    emitChangeSignals(oldValue, m_value);
    return m_value;
}

int Counter::reset() {
    int oldValue = m_value;
    m_value = m_initialValue;

    // Emit reset signal
    emit("reset");

    // If the value actually changed, emit change signals too
    if (oldValue != m_value) {
        emitChangeSignals(oldValue, m_value);
    }

    return m_value;
}

bool Counter::isAtMinimum() const {
    return m_minValue.has_value() && m_value == *m_minValue;
}

bool Counter::isAtMaximum() const {
    return m_maxValue.has_value() && m_value == *m_maxValue;
}

std::optional<int> Counter::getMinValue() const { return m_minValue; }

std::optional<int> Counter::getMaxValue() const { return m_maxValue; }

bool Counter::setMinValue(std::optional<int> min) {
    // Check that min <= max (if max is set)
    if (min.has_value() && m_maxValue.has_value() && *min > *m_maxValue) {
        ArgumentPack args;
        args.add<std::string>(
            "Minimum value cannot be greater than maximum value");
        emit("warn", args);
        return false;
    }

    m_minValue = min;

    // If current value is less than new min, adjust it
    if (min.has_value() && m_value < *min) {
        int oldValue = m_value;
        m_value = *min;
        emitChangeSignals(oldValue, m_value);
    }

    return true;
}

bool Counter::setMaxValue(std::optional<int> max) {
    // Check that max >= min (if min is set)
    if (max.has_value() && m_minValue.has_value() && *max < *m_minValue) {
        ArgumentPack args;
        args.add<std::string>(
            "Maximum value cannot be less than minimum value");
        emit("warn", args);
        return false;
    }

    m_maxValue = max;

    // If current value is greater than new max, adjust it
    if (max.has_value() && m_value > *max) {
        int oldValue = m_value;
        m_value = *max;
        emitChangeSignals(oldValue, m_value);
    }

    return true;
}

bool Counter::isInRange(int value) const {
    if (m_minValue.has_value() && value < *m_minValue) {
        return false;
    }
    if (m_maxValue.has_value() && value > *m_maxValue) {
        return false;
    }
    return true;
}

void Counter::emitChangeSignals(int oldValue, int newValue) {
    // Don't emit signals if value didn't change
    if (oldValue == newValue) {
        return;
    }

    // Emit valueChanged signal with old and new values
    ArgumentPack changeArgs;
    changeArgs.add<int>(oldValue);
    changeArgs.add<int>(newValue);
    emit("valueChanged", changeArgs);

    // Log the change
    ArgumentPack logArgs;
    logArgs.add<std::string>("Counter value changed from " +
                             std::to_string(oldValue) + " to " +
                             std::to_string(newValue));
    emit("log", logArgs);

    // Check if we hit a limit
    if (isAtMinimum() || isAtMaximum()) {
        ArgumentPack limitArgs;
        limitArgs.add<bool>(isAtMinimum()); // true for min, false for max
        limitArgs.add<int>(m_value);
        emit("limitReached", limitArgs);

        // Log the limit reached
        ArgumentPack limitLogArgs;
        limitLogArgs.add<std::string>(
            "Counter reached " +
            std::string(isAtMinimum() ? "minimum" : "maximum") +
            " value: " + std::to_string(m_value));
        emit("log", limitLogArgs);
    }
}