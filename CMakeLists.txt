
cmake_minimum_required(VERSION 3.16)

project(CanProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(canbus src/CanBus.cpp)

target_include_directories(canbus PUBLIC include)

add_executable(test_can tests/test_can.cpp)

target_link_libraries(test_can PRIVATE canbus)

enable_testing()

add_test(NAME can_tests COMMAND test_can)
