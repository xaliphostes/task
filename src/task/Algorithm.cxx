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

#include <task/Algorithm.h>

Algorithm::Algorithm() : m_dirty(true),
                         m_stopRequested(false),
                         m_isRunning(false) {}

bool Algorithm::stopRequested() const { return m_stopRequested; }
void Algorithm::stop() { m_stopRequested = true; }
bool Algorithm::isRunning() const { return m_isRunning; }
bool Algorithm::isDirty() const { return m_dirty; }

void Algorithm::setDirty(bool dirty)
{
    m_dirty = dirty;
    if (dirty)
    {
        m_stopRequested = true;
    }
}

// Version asynchrone de run qui retourne un std::future
std::future<void> Algorithm::run(const std::vector<std::any> &args)
{
    return std::async(std::launch::async, [this, args]()
                      { this->runImpl(args); });
}

void Algorithm::runImpl(const std::vector<std::any> &args)
{
    if (m_isRunning)
    {
        if (m_dirty)
        {
            m_stopRequested = true;
        }
        return;
    }

    setDirty(false);
    m_stopRequested = false;
    m_isRunning = true;

    emit("started");

    try
    {
        exec(args);
    }
    catch (...)
    {
        m_isRunning = false;
        emit("error", std::vector<std::any>{std::string("Exception occurred during algorithm execution")});
        throw; // Re-throw the exception
    }

    m_isRunning = false;
    emit("finished");
}
