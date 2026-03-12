cmake_minimum_required(VERSION 3.20)
project(Server)

set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(NOT SOURCES)
    message(FATAL_ERROR "No source files found in src directory!")
endif()

add_executable(Server ${SOURCES})

target_include_directories(Server PRIVATE include src)

target_link_libraries(Server 
    PRIVATE 
    unofficial::uwebsockets::uwebsockets
    nlohmann_json::nlohmann_json
    $<$<PLATFORM_ID:Windows>:ws2_32>
)
