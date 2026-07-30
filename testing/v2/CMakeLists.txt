cmake_minimum_required(VERSION 3.15)
include(CTest)

project(ObjectSlots_Testing_v2 LANGUAGES CXX)

find_package(Catch2 REQUIRED)
include(Catch)

add_executable(test_basic_v2 test_basic_v2.cpp)
target_link_libraries(test_basic_v2
  PRIVATE
    ObjectSlots
    Catch2::Catch2WithMain
  )
catch_discover_tests(test_basic_v2 DISCOVERY_TIMEOUT 30)

add_executable(test_performance_v2 test_performance_v2.cpp)
target_link_libraries(test_performance_v2
  PRIVATE
    ObjectSlots
    Catch2::Catch2WithMain
  )
catch_discover_tests(test_performance_v2 DISCOVERY_TIMEOUT 30)

add_executable(test_segfault_stress_v2 test_segfault_stress_v2.cpp)
target_link_libraries(test_segfault_stress_v2
  PRIVATE
    ObjectSlots
    Catch2::Catch2WithMain
  )
catch_discover_tests(test_segfault_stress_v2 DISCOVERY_TIMEOUT 30)
