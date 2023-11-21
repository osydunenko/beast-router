set(SRCS main.cpp)

add_executable(ssl-hello_world ${SRCS})

target_link_libraries(
    ssl-hello_world
    PRIVATE
        ${PROJECT_NAME}::${PROJECT_NAME}
        ${PROJECT_NAME}::compile_options
)