OPTION (COVERAGE "Enable gcda file generation needed by lcov" OFF)
OPTION (CPPZMQ_TEST_SENDER "Enable C++26 std::execution support test cases. This requires Boost.Asio" ON)

include(cmake/CPM.cmake)

find_package(Threads)

find_package(Catch2 QUIET)
if (NOT Catch2_FOUND)
  CPMAddPackage(
    NAME Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.3
  )
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

if(CPPZMQ_TEST_SENDER)
    target_compile_features(
      unit_tests PRIVATE cxx_std_26
    )
    target_compile_definitions(
      unit_tests PRIVATE
      CPPZMQ_ENABLE_STDEXEC
    )
    find_package(Boost CONFIG REQUIRED COMPONENTS asio)
    CPMAddPackage(
      NAME stdexec
      GITHUB_REPOSITORY NVIDIA/stdexec
      GIT_TAG nvhpc-25.03.rc1
      DOWNLOAD_ONLY TRUE
    )
    if(NOT stdexec_ADDED)
      message(FATAL_ERROR "The dependency `stdexec` isn't added.")
    endif()
    target_link_libraries(
        unit_tests
        PRIVATE
          Boost::asio
    )
    target_include_directories(
        unit_tests
        PRIVATE "${stdexec_SOURCE_DIR}/include"
    )
    target_sources(
      unit_tests PRIVATE
      async/message.cpp
      async/common.hpp
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
