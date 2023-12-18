cmake_minimum_required(VERSION 3.16)

project(beast_router VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Options decl.
option(BUILD_EXAMPLES "Build examples"     OFF)
option(ROUTER_DOXYGEN "Generate Doxygen"   OFF)
option(BUILD_TESTS    "Build tests"        OFF)
option(LINK_SSL       "Build with openssl" OFF)
option(LINK_ASAN      "Build with asan"    OFF)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Check prereqs
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Doxygen)

if (LINK_SSL)
    find_package(OpenSSL COMPONENTS Crypto SSL)
endif()

find_program(CLANGFORMAT_PATH clang-format)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        Boost::system
        Boost::thread
        $<$<BOOL:${LINK_SSL}>:
            OpenSSL::Crypto OpenSSL::SSL>)

target_compile_definitions(${PROJECT_NAME}
    INTERFACE
        $<$<BOOL:${LINK_SSL}>:LINK_SSL>
        BOOST_ASIO_NO_DEPRECATED
        BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

target_compile_features(${PROJECT_NAME}
    INTERFACE
        cxx_std_17)

add_library(compile_options INTERFACE)
add_library(${PROJECT_NAME}::compile_options ALIAS compile_options)

target_compile_options(compile_options
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
        $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>)

target_link_libraries(compile_options
    INTERFACE
        $<$<BOOL:${LINK_ASAN}>:-fsanitize=address>)

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
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/beast_router/
        COMMENT "Generating API Documentation..."
        VERBATIM)
endif()

# Decl. clang-format target
file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/include/*.ipp)
file(GLOB_RECURSE EX_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp)
file(GLOB_RECURSE TST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
add_custom_target(${PROJECT_NAME}_clangformat
    COMMAND ${CLANGFORMAT_PATH} -i --style=WebKit ${SRCS} ${EX_SRCS} ${TST_SRCS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Fomatting the code with ${CLANGFORMAT_PATH}..."
    VERBATIM)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -DROUTER_DOXYGEN=ON
        -DBUILD_EXAMPLES=ON
        -DBUILD_TESTS=ON
        -DLINK_ASAN=ON
        -DLINK_SSL=ON
        -DCMAKE_BUILD_TYPE=Debug
        ${PROJECT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}
    USES_TERMINAL
    VERBATIM)

# Exclude the clangformat targets from all
set_target_properties(${PROJECT_NAME}_clangformat PROPERTIES EXLCLUDE_FROM_ALL TRUE)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)
