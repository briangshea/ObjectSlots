cmake_minimum_required(VERSION 3.15)

project(ObjectSlots)

option(OBJECTSLOTS_ENABLE_THREADS "Enables the use of theads to emit signal." ON)
option(OBJECTSLOTS_ENABLE_THREAD_SAFETY " Enable thread safety" ON)

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

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        $<$<BOOL:${OBJECTSLOTS_ENABLE_THREADS}>:OBJECTSLOTS_ENABLE_THREADS>
        $<$<BOOL:${OBJECTSLOTS_ENABLE_THREAD_SAFETY}>:OBJECTSLOTS_ENABLE_THREAD_SAFETY>
)

add_subdirectory(testing)
