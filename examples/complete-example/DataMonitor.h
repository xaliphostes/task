#pragma once
#include <task/PeriodicTask.h>

// Task for monitoring directories for new data files
class DataMonitor : public PeriodicTask {
  public:
    DataMonitor(const std::string &directory, int pollIntervalSeconds = 5)
        : PeriodicTask(std::chrono::seconds(pollIntervalSeconds)),
          m_directory(directory) {
        createSignal("newFiles");
    }

  protected:
    void executeImpl() override {
        std::vector<fs::path> newFiles;

        for (const auto &entry : fs::directory_iterator(m_directory)) {
            if (entry.is_regular_file() &&
                m_processedFiles.find(entry.path()) == m_processedFiles.end()) {
                newFiles.push_back(entry.path());
                m_processedFiles.insert(entry.path());
            }
        }

        if (!newFiles.empty()) {
            emit("newFiles", ArgumentPack(newFiles));

            emitString("log", "Found " + std::to_string(newFiles.size()) +
                                  " new files to process");
        }
    }

  private:
    std::string m_directory;
    std::set<fs::path> m_processedFiles;
};
