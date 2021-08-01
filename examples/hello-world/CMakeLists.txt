set(SRCS main.cpp)

add_executable(hello_world ${SRCS})

target_link_libraries(
    hello_world
    PRIVATE
        beast_router::beast_router
)
