set(SRCS main.cpp)

add_executable(ssl-hello_world ${SRCS})

target_link_libraries(
    ssl-hello_world
    PRIVATE
        beast_router::beast_router
        beast_router::compile_options
)