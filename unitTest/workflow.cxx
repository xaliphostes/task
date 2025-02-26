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
#include <task/Chronometer.h>
#include <task/Logger.h>
#include <task/Trigger.h>

// LongTask.h
class LongTask : public Algorithm {
  public:
    void exec(const Args &args = {}) override {
        emit("warn", Args{std::string("LongTask is running...")});
        std::this_thread::sleep_for(std::chrono::seconds(1));
        emit("log", Args{std::string("...done.")});
        emit("finished");
    }
};

// VeryLongTask.h
class VeryLongTask : public Algorithm {
  public:
    void exec(const Args &args = {}) override {
        emit("warn", Args{std::string("VeryLongTask is running...")});
        std::this_thread::sleep_for(std::chrono::seconds(3));
        emit("log", Args{std::string("...done.")});
        emit("finished");
    }
};

// View.h
class View : public Task {
  public:
    explicit View(int id) : m_id(id) {}

    void update(const Args &args) {
        emit("log",
             Args{std::string("    Updating View") + std::to_string(m_id)});
    }

  private:
    int m_id;
};

// Workflow.h
class Workflow : public Task {
  public:
    Workflow() {
        // Création des composants
        m_log = std::make_shared<Logger>("--->");
        m_timer = std::make_shared<Chronometer>();
        m_longTask = std::make_shared<LongTask>();
        m_veryLongTask = std::make_shared<VeryLongTask>();

        // Création des vues
        for (int i = 0; i < 3; ++i) {
            m_views.push_back(std::make_shared<View>(i));
        }

        // Connexion des logs
        m_log->connectAllSignalsTo(m_longTask.get());
        m_log->connectAllSignalsTo(m_veryLongTask.get());
        m_log->connectAllSignalsTo(this);
        for (auto &view : m_views) {
            m_log->connectAllSignalsTo(view.get());
        }

        // Connexion des signaux timer
        this->connect("started", [this]() { m_timer->start(); });
        m_veryLongTask->connect("finished", [this]() { m_timer->stop(); });
        m_longTask->connect("finished", [this]() { m_timer->stop(); });

        m_timer->connect("finished", [](const Args &args) {
            if (!args.empty()) {
                auto ms = std::any_cast<int64_t>(args[0]);
                std::cout << "--> Elapsed time: " << ms << " ms" << std::endl;
            }
        });

        // Connexion des tâches
        m_longTask->connect("finished", [this]() { m_veryLongTask->run(); });
        m_veryLongTask->connect("finished", [this]() {
            emit("log", Args{std::string("All tasks are done!")});
        });

        // Connexion des vues
        m_longTask->connect("finished", [this]() { m_views[0]->update({}); });
        m_longTask->connect("finished", [this]() { m_views[2]->update({}); });
        m_veryLongTask->connect("finished",
                                [this]() { m_views[1]->update({}); });
        m_veryLongTask->connect("finished",
                                [this]() { m_views[2]->update({}); });
        m_veryLongTask->connect("finished",
                                []() { std::cout << "END" << std::endl; });
    }

    void start() {
        emit("started");
        m_longTask->run();
    }

  private:
    std::shared_ptr<Logger> m_log;
    std::shared_ptr<Chronometer> m_timer;
    std::shared_ptr<LongTask> m_longTask;
    std::shared_ptr<VeryLongTask> m_veryLongTask;
    std::vector<std::shared_ptr<View>> m_views;
};

// Exemple d'utilisation
int main() {
    auto button = std::make_shared<Trigger>();
    auto workflow = std::make_shared<Workflow>();

    button->connect("tick", [workflow]() { workflow->start(); });

    // Démarrage du workflow
    button->tick();
}
