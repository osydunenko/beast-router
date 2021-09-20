set(SRCS main.cpp)

add_executable(client ${SRCS})

target_link_libraries(
    client
    PRIVATE
        beast_router::beast_router
)
