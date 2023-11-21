set(DOC_BASE_PATH "${CMAKE_CURRENT_BINARY_DIR}/doc")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/config_ex.hpp)

add_custom_target(
    copy-doc ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${CMAKE_CURRENT_SOURCE_DIR}/doc"
        "${DOC_BASE_PATH}"
    DEPENDS time_counter)

add_example(time_counter SOURCES main.cpp)
