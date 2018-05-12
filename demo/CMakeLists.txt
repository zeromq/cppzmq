cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cppzmq-demo CXX)

find_package(cppzmq REQUIRED)

enable_testing()
add_executable(
    demo
    main.cpp
    )

target_link_libraries(
    demo
    cppzmq
    )

target_include_directories(demo PRIVATE ${cppzmq_INCLUDE_DIR}
                                        ${ZeroMQ_INCLUDE_DIR})

add_test(
  NAME
    demo
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/demo
  )
