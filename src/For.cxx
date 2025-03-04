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

#include <task/For.h>

ForParameters::ForParameters(int start_, int stop_, int step_)
    : start(start_), stop(stop_), step(step_) {}

// ---------------------------------------------

For::For(const ForParameters &params)
    : m_start(0), m_stop(10), m_step(1), m_current(0) {
    createSignal("tick"); // This line is likely missing!
    set(params);
}

// Configuration de la boucle
void For::set(const ForParameters &params) {
    if (params.start)
        m_start = *params.start;
    if (params.stop)
        m_stop = *params.stop;
    if (params.step)
        m_step = *params.step;

    if (m_start > m_stop && m_step > 0) {
        emitString("warn", "Bad configuration of the ForLoop");
    }
}

// Getters et setters
int For::startValue() const { return m_start; }
void For::setStartValue(int value) { m_start = value; }

int For::stopValue() const { return m_stop; }
void For::setStopValue(int value) { m_stop = value; }

int For::stepValue() const { return m_step; }
void For::setStepValue(int value) { m_step = value; }

int For::getCurrentValue() const { return m_current; }

// Démarrage de la boucle
void For::start() {
    for (m_current = m_start; m_current != m_stop; m_current += m_step) {
        // Create an ArgumentPack with all loop information
        ArgumentPack tickData;
        tickData.add<int>(m_start);   // Index 0: start
        tickData.add<int>(m_stop);    // Index 1: stop
        tickData.add<int>(m_current); // Index 2: current
        tickData.add<int>(m_step);    // Index 3: step

        // Emit the tick signal with the loop data
        emit("tick", tickData);
    }
}

std::future<void> For::startAsync() {
    return std::async(std::launch::async, [this]() { this->start(); });
}