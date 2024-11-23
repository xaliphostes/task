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

#include <task/SignalSlot.h>

void SignalSlot::createSignal(const std::string& signal) {
    if (hasSignal(signal)) {
        std::cerr << "Signal '" << signal << "' already exists" << std::endl;
        return;
    }
    m_signals[signal] = {};
}

void SignalSlot::emit(const std::string& signal, const std::vector<std::any>& args) {
    if (!hasSignal(signal)) {
        std::cerr << "Signal '" << signal << "' not found" << std::endl;
        return;
    }

    for (const auto& connection : m_signals[signal]) {
        connection->trigger(args);
    }
}

bool SignalSlot::hasSignal(const std::string& signal) const {
    return m_signals.find(signal) != m_signals.end();
}