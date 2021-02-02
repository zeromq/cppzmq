cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cppzmq-examples CXX)

# place binaries and libraries according to GNU standards

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

find_package(Threads)
find_package(cppzmq)

add_executable(
    pubsub_multithread_inproc
    pubsub_multithread_inproc.cpp
)
target_link_libraries(
    pubsub_multithread_inproc
    PRIVATE cppzmq ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(
    hello_world
    hello_world.cpp
)
target_link_libraries(
    hello_world
    PRIVATE cppzmq ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(
    multipart_messages
    multipart_messages.cpp
)
target_link_libraries(
    multipart_messages
    PRIVATE cppzmq ${CMAKE_THREAD_LIBS_INIT}
)
