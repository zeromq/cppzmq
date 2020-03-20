cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cppzmq-test CXX)

# place binaries and libraries according to GNU standards

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

include(CTest)
include(cmake/catch.cmake)
include(${CATCH_MODULE_PATH}/Catch.cmake)

find_package(Threads)

add_executable(
    unit_tests
    buffer.cpp
    message.cpp
    context.cpp
    socket.cpp
    socket_ref.cpp
    poller.cpp
    active_poller.cpp
    multipart.cpp
    recv_multipart.cpp
    send_multipart.cpp
    codec_multipart.cpp
    monitor.cpp
    utilities.cpp
)

add_dependencies(unit_tests catch)

target_include_directories(unit_tests PUBLIC ${CATCH_MODULE_PATH})
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

catch_discover_tests(unit_tests)
