cmake_minimum_required(VERSION 3.15)

project(ObjectSlots_Testing)

add_executable(${PROJECT_NAME} 
    test_slots.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ObjectSlots
)
