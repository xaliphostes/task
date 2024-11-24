# Copyright (c) 2024-now fmaerten@gmail.com
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
cmake_minimum_required(VERSION 3.5)
project(task)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS on)

set(DIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if(ENABLE_COVERAGE)
  # set compiler flags
  set(CMAKE_CXX_FLAGS "-O0 -coverage")

  # find required tools
  find_program(LCOV lcov REQUIRED)
  find_program(GENHTML genhtml REQUIRED)

  # add coverage target
  add_custom_target(coverage
    # gather data
    COMMAND ${LCOV} --directory . --capture --output-file coverage.info
    # generate report
    # --ignore-errors inconsistent 
    COMMAND ${GENHTML}--demangle-cpp -o coverage coverage.info
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

IF (WIN32)
    add_definitions( -TASK_EXPORTS=TRUE )
ENDIF()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message("Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
else()
    message("Setting build type to 'Debug' (specified in cli).")
endif()

include_directories(include)

set(SRC_task
    ./src/task/SignalSlot.cxx
    ./src/task/Task.cxx
    ./src/task/Algorithm.cxx
    ./src/task/Logger.cxx
    ./src/task/For.cxx
    ./src/task/FlowAlgorithm.cxx
    ./src/task/ParallelAlgorithm.cxx
    ./src/task/Trigger.cxx
    ./src/task/Chronometer.cxx

    ./src/task/concurrent/Runnable.cxx
    ./src/task/concurrent/ThreadPool.cxx
)

add_library(task-shared SHARED ${SRC_task})
set_target_properties(task-shared PROPERTIES OUTPUT_NAME Task)

set_target_properties(task-shared PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${DIST_DIR})
set_target_properties(task-shared PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${DIST_DIR})
set_target_properties(task-shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})
set_target_properties(task-shared PROPERTIES PDB_OUTPUT_DIRECTORY ${DIST_DIR})

# Testing...
enable_testing()
add_subdirectory(unitTest)
add_subdirectory(examples/pi-1)
add_subdirectory(examples/pi-2)
