project(counter)

add_executable(${PROJECT_NAME} main.cxx)

target_link_libraries(${PROJECT_NAME}
    task-shared
)

set_target_properties(counter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})
