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

#include <any>
#include <vector>

// A simple algorithm that demonstrates the signal-slot system
inline Algorithm::Algorithm()
    : m_dirty(true), m_stopRequested(false), m_isRunning(false) {
    // Add a progress signal that takes a float parameter
    createDataSignal("progress");
}

inline bool Algorithm::stopRequested() const { return m_stopRequested; }

inline void Algorithm::stop() { m_stopRequested = true; }

inline bool Algorithm::isRunning() const { return m_isRunning; }

inline bool Algorithm::isDirty() const { return m_dirty; }

inline void Algorithm::setDirty(bool dirty) {
    m_dirty = dirty;
    if (dirty) {
        m_stopRequested = true;
    }
}

// Report progress (0.0 to 1.0)
inline void Algorithm::reportProgress(float progress) {
    ArgumentPack args;
    args.add<float>(progress);
    emit("progress", args);
}

// Asynchronous execution with variable arguments
inline std::future<void> Algorithm::run(const ArgumentPack &args) {
    auto argsClone = args.clone();
    return std::async(std::launch::async,
                      [this, argsClone = std::move(argsClone)]() { this->runImpl(argsClone); });
}

inline void Algorithm::runImpl(const ArgumentPack &args) {
    if (m_isRunning) {
        if (m_dirty) {
            m_stopRequested = true;
        }
        return;
    }

    setDirty(false);
    m_stopRequested = false;
    m_isRunning = true;

    emit("started");

    try {
        exec(args);
    } catch (const std::exception &e) {
        m_isRunning = false;
        emitString("error", e.what());
        throw; // Re-throw the exception
    } catch (...) {
        m_isRunning = false;
        emitString("error", "Unknown exception during algorithm execution");
        throw; // Re-throw the exception
    }

    m_isRunning = false;
    emit("finished");
}