cmake_minimum_required(VERSION 3.16)

project(beast_router)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Options decl.
option(BUILD_EXAMPLES "Build examples" OFF)
option(ROUTER_DOXYGEN "Generate Doxygen" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(LINK_SSL "Build with openssl" OFF)
option(LINK_ASAN "Build with asan" OFF)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Add compile options
add_library(common_options INTERFACE)
add_library(${PROJECT_NAME}::common_options ALIAS common_options)
target_compile_options(
    common_options
    INTERFACE
        -Werror
        -Wextra
        -Wpedantic
        -Wunused-variable
        $<$<BOOL:${LINK_ASAN}>:
            -fsanitize=address 
            -fno-optimize-sibling-calls
            -fsanitize-address-use-after-scope
            -fno-omit-frame-pointer>
        $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>
)

add_library(common_libraries INTERFACE)
add_library(${PROJECT_NAME}::common_libraries ALIAS common_libraries)
target_link_libraries(
    common_libraries
    INTERFACE
        $<$<BOOL:${LINK_ASAN}>:-fsanitize=address>
)

add_library(common_definitions INTERFACE)
add_library(${PROJECT_NAME}::common_definitions ALIAS common_definitions)
target_compile_definitions(
    common_definitions
    INTERFACE
        BOOST_BEAST_USE_STD_STRING_VIEW
        BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT
)

# Check prereqs
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Doxygen)
find_package(OpenSSL COMPONENTS Crypto SSL)
find_program(CLANGFORMAT_PATH clang-format)

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
        ${PROJECT_NAME}::common_options
        ${PROJECT_NAME}::common_libraries
        ${PROJECT_NAME}::common_definitions
        $<$<BOOL:${LINK_SSL}>:OpenSSL::Crypto OpenSSL::SSL>
)

target_compile_definitions(
    ${PROJECT_NAME}
    INTERFACE
        $<$<BOOL:${LINK_SSL}>:LINK_SSL>
)

if (ROUTER_DOXYGEN)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "The project is being built with the 'ROUTER_DOXYGEN=ON' but the doxygen binearies are not found")
    endif()

    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile)
    set(DOXYGEN_DOC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doc)
    set(DOXYGEN_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    set(DOXYGEN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/beast_router/)
    set(DOXYGEN_INPUT "${DOXYGEN_README} ${DOXYGEN_FILES}")

    # configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY NEWLINE_STYLE UNIX)

    # Decl. doc target
    add_custom_target(
        doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/beast_router/
        COMMENT "Generating API Documentation"
        VERBATIM
    )
endif()

# Decl. clang-format target
file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/include/*)
file(GLOB_RECURSE EX_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp)
file(GLOB_RECURSE TST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
add_custom_target(
    ${PROJECT_NAME}_clangformat
    COMMAND ${CLANGFORMAT_PATH} -i --style=Google ${SRCS} ${EX_SRCS} ${TST_SRCS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Fomatting the code with ${CLANGFORMAT_PATH}"
    VERBATIM
)
# Exclude the coverage target from all
set_target_properties(${PROJECT_NAME}_clangformat PROPERTIES EXLCLUDE_FROM_ALL TRUE)

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
message(STATUS "ROUTER_DOXYGEN:..............${ROUTER_DOXYGEN}")
message(STATUS "LINK_SSL:....................${LINK_SSL}")
message(STATUS "CMAKE_BUILD_TYPE:............${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_VERSION:...............${CMAKE_VERSION}")
message(STATUS "CMAKE_C_COMPILER:............${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:..........${CMAKE_CXX_COMPILER}")
message(STATUS "DOXYGEN_EXECUTABLE:..........${DOXYGEN_EXECUTABLE}")    
message(STATUS "BOOST VERSION:...............${Boost_VERSION_STRING}")    
message(STATUS "-------------------------------------------------------")
