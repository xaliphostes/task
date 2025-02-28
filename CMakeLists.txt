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

function (auto_discover_tests)
    file(GLOB_RECURSE BASE_TEST_FILES "unitTest/*_test.cxx")
    foreach(TEST_FILE ${BASE_TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE task-shared)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach() 
endfunction()

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
# add_subdirectory(src)
file(GLOB_RECURSE SRC_task "${PROJECT_SOURCE_DIR}/src/*.cxx")
add_library(task-shared SHARED ${SRC_task})

set_target_properties(task-shared PROPERTIES OUTPUT_NAME Task)

set_target_properties(task-shared PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${DIST_DIR})
set_target_properties(task-shared PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${DIST_DIR})
set_target_properties(task-shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})
set_target_properties(task-shared PROPERTIES PDB_OUTPUT_DIRECTORY ${DIST_DIR})

enable_testing()
auto_discover_tests()

add_subdirectory(examples/concurrent)
add_subdirectory(examples/counter)
add_subdirectory(examples/parallel)
add_subdirectory(examples/pi-1)
add_subdirectory(examples/pi-2)
