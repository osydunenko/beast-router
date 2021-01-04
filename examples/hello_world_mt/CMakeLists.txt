find_package(Boost REQUIRED COMPONENTS system thread)

set(OUTPUT_NAME ex_hello_world_mt)

add_executable(${OUTPUT_NAME} main.cpp)

target_include_directories(
    ${OUTPUT_NAME}
    PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(
    ${OUTPUT_NAME}
    ${Boost_LIBRARIES}
)
