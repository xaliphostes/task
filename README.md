# Task

<p align="center">
  <img src="https://img.shields.io/static/v1?label=Linux&logo=linux&logoColor=white&message=support&color=success" alt="Linux support">
  <img src="https://img.shields.io/static/v1?label=macOS&logo=apple&logoColor=white&message=support&color=success" alt="macOS support">
  <img src="https://img.shields.io/static/v1?label=Windows&logo=windows&logoColor=white&message=soon&color=red" alt="Windows support">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="Language">
  <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License">
  
</p>

Minimalist Reactive library in C++

## Requirements
- C++20 (but C++23 soon)
- cmake

# Compilation
Create a `build` directory, **go inside** and type
```sh
cmake .. && make -j12
```

# Running unit tests
**NOTE**: The internal cmake test is used to perform unit testing.

In the **same directory** as for the compilation (i.e., the `build` directory), only type
```sh
ctest
```
or
```sh
make test
```

# Example for an algorithm
```cpp
#include <task/Task.h>
#include <task/Algorithm.h>

class ExampleAlgorithm : public Algorithm {
public:
    void exec(const Args& args = {}) override {
        emit("log", Args{std::string("Starting algorithm execution")});
        
        // Simulating a long run
        for (int i = 0; i < 100; ++i) {
            if (stopRequested()) {
                emit("log", Args{std::string("Algorithm stopped by user")});
                return;
            }

            // Do something long...
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            emit("log", Args{
                std::string("Processing: ") + std::to_string(i) + "%"
            });
        }
        
        emit("log", Args{std::string("Algorithm completed")});
    }
};

int main() {
    ExampleAlgorithm algo;
    
    // Connecting the signals
    algo.connect("started", [](const Args& args) {
        std::cout << "Algorithm started" << std::endl;
    });

    algo.connect("finished", [](const Args& args) {
        std::cout << "Algorithm finished" << std::endl;
    });

    algo.connect("log", [](const Args& args) {
        if (!args.empty()) {
            try {
                std::cout << std::any_cast<std::string>(args[0]) << std::endl;
            } catch (const std::bad_any_cast&) {
                std::cout << "Invalid log format" << std::endl;
            }
        }
    });

    // Run the algo asynchronously
    auto future = algo.run();

    // Possibility to stop the algorithm
    std::this_thread::sleep_for(std::chrono::seconds(2));
    algo.stop();

    // Waiting for the algo to end
    future.wait();
}
```

## Licence
MIT

## Contact
fmaerten@gmail.com
