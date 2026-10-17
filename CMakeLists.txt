
cmake_minimum_required(VERSION 3.16)

project(CanProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(canbus
	src/SocketCanFdBus.cpp
	src/EcuFdSimulator.cpp
	src/EcuSimulator.cpp
	src/SocketCanBus.cpp
)

target_include_directories(canbus PUBLIC include)

add_executable(test_can tests/test_can.cpp)

target_link_libraries(test_can PRIVATE canbus)

add_executable(test_canfd tests/test_canfd.cpp)

target_link_libraries(test_canfd PRIVATE canbus)

add_executable(vehicle_infotainment examples/vehicle_infotainment.cpp)

target_link_libraries(vehicle_infotainment PRIVATE canbus)

add_executable(vehicle_infotainment_fd examples/vehicle_infotainment_fd.cpp)

target_link_libraries(vehicle_infotainment_fd PRIVATE canbus)

add_executable(arduino_rtr examples/arduino_rtr.cpp)

target_link_libraries(arduino_rtr PRIVATE canbus)

enable_testing()

add_test(NAME can_tests COMMAND test_can)
add_test(NAME canfd_tests COMMAND test_canfd)
