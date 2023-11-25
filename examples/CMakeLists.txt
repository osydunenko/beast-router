include(CMakeParseArguments)

function(add_example TARGET)
    message(STATUS "Adding example: ${TARGET}")
    set(MULTI_VALUE_ARGS SOURCES)

    cmake_parse_arguments(
        EXAMPLE
            ""
            ""
            "${MULTI_VALUE_ARGS}"
            ${ARGN})
    
    add_executable(${TARGET} ${EXAMPLE_SOURCES})
    target_link_libraries(${TARGET}
        PRIVATE
            ${PROJECT_NAME}::${PROJECT_NAME}
            ${PROJECT_NAME}::compile_options)
endfunction()

add_subdirectory(hello-world)
add_subdirectory(time-counter)
add_subdirectory(client)

if (LINK_SSL)
    add_subdirectory(ssl/hello-world)
    add_subdirectory(ssl/client)
endif()
