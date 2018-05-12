cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(cppzmq-demo CXX)

find_package(cppzmq)

enable_testing()
add_executable(
    demo
    main.cpp
    )

target_link_libraries(
    demo
    cppzmq
    )

add_test(
  NAME
    demo
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/demo
  )
