OPTION (COVERAGE "Enable gcda file generation needed by lcov" OFF)
OPTION (CPPZMQ_TEST_COROUTINE "Enable C++20 coroutine support test cases. This requires Boost.Asio" ON)

find_package(Threads)

find_package(Catch2 QUIET)

if (NOT Catch2_FOUND)
  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.3)

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

if (CPPZMQ_TEST_COROUTINE)
    target_compile_features(
      unit_tests PRIVATE cxx_std_23
    )
    target_compile_definitions(
      unit_tests PRIVATE
      CPPZMQ_ENABLE_CORRAL_COROUTINE
    )
    find_package(Boost CONFIG REQUIRED COMPONENTS asio)
    target_include_directories(unit_tests PRIVATE third_party/corral)
        target_link_libraries(
        unit_tests
        PRIVATE Boost::asio
    )
    target_sources(
      unit_tests PRIVATE
      async/corral/message.cpp
      async/corral/common.hpp
    )
endif()

target_include_directories(unit_tests PUBLIC ${CATCH_MODULE_PATH})
target_link_libraries(
    unit_tests
    PRIVATE Catch2::Catch2WithMain
    PRIVATE cppzmq
    PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)


if (COVERAGE)
    target_compile_options(unit_tests PRIVATE --coverage)
    target_link_options(unit_tests PRIVATE --coverage)
    message(STATUS "Coverage enabled")
endif()

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
