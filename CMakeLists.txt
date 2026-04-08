cmake_minimum_required(VERSION 3.30)

project(qtoxchat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep Qt-related autogen enabled so adding Qt targets later stays simple.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(CURL CONFIG REQUIRED)

qt_add_executable(qtoxchat
    src/main.cpp
)

target_link_libraries(qtoxchat
    PRIVATE
        Qt6::Widgets
        CURL::libcurl
)

install(TARGETS qtoxchat)
