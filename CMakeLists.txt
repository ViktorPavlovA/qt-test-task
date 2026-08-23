cmake_minimum_required(VERSION 3.16)

project(NPOKalibri_test_task VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer Qt6
find_package(Qt6 6.5.2 REQUIRED COMPONENTS Core Network Widgets Gui)

add_subdirectory(server)
add_subdirectory(client)

message(STATUS "Qt System configured successfully (Qt ${Qt6_VERSION})")



