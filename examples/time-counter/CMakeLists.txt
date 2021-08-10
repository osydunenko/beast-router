set(SRCS main.cpp)

add_executable(time_counter ${SRCS})

target_link_libraries(
    time_counter
    PRIVATE
        beast_router::beast_router
)

set(DOC_BASE_PATH "${CMAKE_CURRENT_BINARY_DIR}/doc")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp)

add_custom_target(
    copy-doc ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${CMAKE_CURRENT_SOURCE_DIR}/doc"
        "${DOC_BASE_PATH}"
    DEPENDS time_counter
)
