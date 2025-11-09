cmake_minimum_required(VERSION 3.15)

project(ObjectSlots)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Ensures compilation fails if C++17 is not supported
set(CMAKE_CXX_EXTENSIONS OFF)      # Disables compiler-specific extensions (optional)

add_library(${PROJECT_NAME} OBJECT 
    include/ObjectSlots/ObjectSlots.hpp
    src/ObjectSlots.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC include
)

add_subdirectory(testing)
