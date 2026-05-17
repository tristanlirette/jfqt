cmake_minimum_required(VERSION 3.16)
project(jfqt LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
enable_testing()

qt6_add_executable(jfqt
    src/main.cpp
    src/line_parser.cpp
    src/jft_process.cpp
    src/main_window.cpp
)
target_link_libraries(jfqt PRIVATE Qt6::Widgets)

install(TARGETS jfqt)

add_subdirectory(tests)
