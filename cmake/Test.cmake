include(CTest)
include(CMakeParseArguments)

function(add_unit_test TARGET)
    set(multi_value_args SOURCES)

    cmake_parse_arguments(
        TEST
        ""
        ""
        "${multi_value_args}"
        ${ARGN}
    )

    message(STATUS "Adding test \"${TARGET}\"")

    if (NOT TEST_SOURCES)
        set(TEST_SOURCES "${TARGET}.cpp")
    endif()

    add_executable(${TARGET} ${TEST_SOURCES})

    target_compile_definitions(
        ${TARGET}
        PRIVATE
            BOOST_TEST_DYN_LINK
            BOOST_TEST_MODULE="${TARGET}"
    )

    target_link_libraries(
        ${TARGET}
        PRIVATE
            beast_router::beast_router
            beast_router::compile_options
            Boost::unit_test_framework
    )

    add_test(
        NAME ${TARGET}
        COMMAND $<TARGET_FILE:${TARGET}>
    )

endfunction()

find_package(Boost REQUIRED COMPONENTS unit_test_framework)