cmake_minimum_required(VERSION 3.30)

project(qtoxchat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Keep Qt-related autogen enabled so adding Qt targets later stays simple.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ConfigureClangd)
include(Dependencies)
include(Toxcore)


set(CORE_INCLUDE_FILES 
    ${CMAKE_SOURCE_DIR}/include/core/toxcore_wrapper.h
)
set(CORE_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/core/toxcore_wrapper.cpp
)

qt_add_executable(${PROJECT_NAME}
    src/main.cpp

    ${CORE_INCLUDE_FILES}
    ${CORE_SOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/c-toxcore
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        CURL::libcurl
        
        toxcore_shared
)

install(TARGETS ${PROJECT_NAME})
