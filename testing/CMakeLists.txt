cmake_minimum_required(VERSION 3.15)

project(ObjectSlots_Testing)

add_executable(${PROJECT_NAME}
    test_slots.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ObjectSlots
)

add_executable(test_segfault test_segfault.cpp)
target_link_libraries(test_segfault PRIVATE ObjectSlots)

add_executable(test_segfault_simple test_segfault_simple.cpp)
target_link_libraries(test_segfault_simple PRIVATE ObjectSlots)

add_executable(test_comprehensive test_comprehensive.cpp)
target_link_libraries(test_comprehensive PRIVATE ObjectSlots)
