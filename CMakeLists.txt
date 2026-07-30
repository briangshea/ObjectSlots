cmake_minimum_required(VERSION 3.15)

project(ObjectSlots VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Ensures compilation fails if C++17 is not supported
set(CMAKE_CXX_EXTENSIONS OFF)       # Disables compiler-specific extensions (optional)

 
option(OBJECTSLOTS_ENABLE_THREADS       "Enables the use of theads to emit signal." ON)
option(OBJECTSLOTS_ENABLE_THREAD_SAFETY "Enable thread safety" ON)
option(ENABLE_OBJECTSLOTS_V1            "Enable v1 API" OFF)
option(ENABLE_OBJECTSLOTS_V2            "Enable v2 API" ON)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ObjectSlots/objectslots_config.hpp.in
    ${CMAKE_BINARY_DIR}/include/ObjectSlots/objectslots_config.hpp
)

if(ENABLE_OBJECTSLOTS_V1)
  message(STATUS "Enabled v1 API")
  add_library(${PROJECT_NAME}_v1 OBJECT src/ObjectSlots.cpp)
  target_include_directories(${PROJECT_NAME}_v1
  PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_BINARY_DIR}/include
  )
endif(ENABLE_OBJECTSLOTS_V1)

if(ENABLE_OBJECTSLOTS_V2)
  message(STATUS "Enabled v2 API")
  if(NOT TARGET ${PROJECT_NAME})
    add_library(${PROJECT_NAME} INTERFACE)
    target_include_directories(${PROJECT_NAME} 
      INTERFACE  
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
  )
  endif()
endif()

add_subdirectory(testing)
