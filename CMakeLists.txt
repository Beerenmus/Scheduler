cmake_minimum_required(VERSION 3.14)

project("Schedeuler")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_executable(schedeuler 
    main.cpp
)
