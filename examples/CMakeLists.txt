
file(GLOB SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ex_*.cpp)

foreach(ex_src ${SRCS})

    get_filename_component(ex_name ${ex_src} NAME_WE)

    message(STATUS "Bulding an example: ${ex_name}")

    add_executable(${ex_name} ${ex_src})

    target_link_libraries(
        ${ex_name}
        PRIVATE
            beast_router::beast_router
    )

endforeach()
