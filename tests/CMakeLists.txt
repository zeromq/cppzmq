cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cppzmq-test CXX)

# place binaries and libraries according to GNU standards

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

include(CTest)
find_package(Catch2 QUIET)
if (Catch2_FOUND)
    message(STATUS "Using external Catch 2: ${Catch2_VERSION}")
    set(CATCH_TYPE "catch2")
    include(Catch)
else()
    find_path(CATCH_HEADER_DIR "catch.hpp")
    if (CATCH_HEADER_DIR)
        message(STATUS "Using external Catch 1 at ${CATCH_HEADER_DIR}")
        set(CATCH_TYPE "catch1")
    else()
        message(STATUS "Using downloaded copy of Catch.")
        include(cmake/catch.cmake)
        include(${CATCH_MODULE_PATH}/Catch.cmake)
        set(CATCH_TYPE "internal")
    endif()
endif()

find_package(Threads)

add_executable(
    unit_tests
    message.cpp
    context.cpp
    socket.cpp
    poller.cpp
    active_poller.cpp
    multipart.cpp
	monitor.cpp
)

if (CATCH_TYPE STREQUAL "catch2")
    target_link_libraries(unit_tests PRIVATE Catch2::Catch2)
    target_compile_definitions(unit_tests PRIVATE -DUSE_EXTERNAL_CATCH2)
elseif(CATCH_TYPE STREQUAL "catch1")
    target_include_directories(unit_tests PRIVATE ${CATCH_HEADER_DIR})
else()
    add_dependencies(unit_tests catch)
    target_include_directories(unit_tests PUBLIC ${CATCH_MODULE_PATH})
endif()

target_link_libraries(
    unit_tests
    PRIVATE cppzmq
    PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)

OPTION (COVERAGE "Enable gcda file generation needed by lcov" OFF)

if (COVERAGE)
    target_compile_options(unit_tests PRIVATE --coverage)
    target_link_libraries(unit_tests PRIVATE --coverage)
endif()

if (CATCH_TYPE STREQUAL "catch1")
    add_test(
      NAME
        unit
      COMMAND
        ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
    )
else()
    catch_discover_tests(unit_tests)
endif()
