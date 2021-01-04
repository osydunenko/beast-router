cmake_minimum_required(VERSION 3.16)

set(PROJECT beast_router)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${PROJECT_SOURCE_DIR}/examples/hello_world")
add_subdirectory("${PROJECT_SOURCE_DIR}/examples/hello_world_mt")
