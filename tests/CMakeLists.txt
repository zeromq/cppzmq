find_package(Threads)

find_package(Catch2 QUIET)

if (NOT Catch2_FOUND)
  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.9)

  FetchContent_MakeAvailable(Catch2)

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
endif()

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

target_include_directories(unit_tests PUBLIC ${CATCH_MODULE_PATH})
target_link_libraries(
    unit_tests
    PRIVATE Catch2::Catch2
    PRIVATE cppzmq
    PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)

OPTION (COVERAGE "Enable gcda file generation needed by lcov" OFF)

if (COVERAGE)
    target_compile_options(unit_tests PRIVATE --coverage)
    target_link_options(unit_tests PRIVATE --coverage)
    message(STATUS "Coverage enabled")
endif()

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
