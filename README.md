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

## Licence
MIT

## Contact
fmaerten@gmail.com
