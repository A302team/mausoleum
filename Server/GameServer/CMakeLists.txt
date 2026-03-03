cmake_minimum_required(VERSION 3.20)
project(Server)

set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(Server src/main.cpp)

target_include_directories(Server PRIVATE src)

target_link_libraries(Server 
    PRIVATE 
    unofficial::uwebsockets::uwebsockets
    nlohmann_json::nlohmann_json
)