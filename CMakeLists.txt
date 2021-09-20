cmake_minimum_required(VERSION 3.16)

project(beast_router)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Options decl.
option(BUILD_EXAMPLES "Build examples" OFF)
option(ROUTER_DOXYGEN "Generate Doxygen" OFF)
option(BUILD_TESTS "Build tests" OFF)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

# Add compile options
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always -Wall -Wextra -pedantic -Werror)
else()
    message(FATAL_ERROR "The compiler \"${CMAKE_CXX_COMPILER_ID}\" is not supported")
endif()

# Check prereqs
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Doxygen)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:Boost::headers>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    INTERFACE
        Boost::system
        Boost::thread
)

if (ROUTER_DOXYGEN)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "The project is being built with the 'ROUTER_DOXYGEN=ON' but the doxygen binearies are not found")
    endif()

    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_DOC_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/doc)

    # configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # Decl. doc target
    add_custom_target(
        doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/beast_router/
        COMMENT "Generating API Documentation"
        VERBATIM
    )

    add_definitions(ROUTER_DOXYGEN=1)

    # Exclude the doc target from all
    set_target_properties(doc PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "-------------------------------------------------------")
message(STATUS "BUILD_EXAMPLES:..............${BUILD_EXAMPLES}")
message(STATUS "BUILD_TESTS:.................${BUILD_TESTS}")
message(STATUS "CMAKE_BUILD_TYPE:............${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_VERSION:...............${CMAKE_VERSION}")
message(STATUS "CMAKE_C_COMPILER:............${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:..........${CMAKE_CXX_COMPILER}")
message(STATUS "DOXYGEN_EXECUTABLE:..........${DOXYGEN_EXECUTABLE}")
message(STATUS "ROUTER_DOXYGEN:..............${ROUTER_DOXYGEN}")
message(STATUS "-------------------------------------------------------")
