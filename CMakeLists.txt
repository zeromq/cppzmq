cmake_minimum_required(VERSION 3.0.0)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include (DetectCPPZMQVersion)

project(cppzmq VERSION ${DETECTED_CPPZMQ_VERSION})

find_package(ZeroMQ QUIET)

# libzmq autotools install: fallback to pkg-config
if(NOT ZeroMQ_FOUND)
    message(STATUS "CMake libzmq package not found, trying again with pkg-config (normal install of zeromq)")
    list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/libzmq-pkg-config)
    find_package(ZeroMQ REQUIRED)
endif()

# TODO "REQUIRED" above should already cause a fatal failure if not found, but this doesn't seem to work
if(NOT ZeroMQ_FOUND)
    message(FATAL_ERROR "ZeroMQ was not found, neither as a CMake package nor via pkg-config")
endif()

if (ZeroMQ_FOUND AND (NOT TARGET libzmq OR NOT TARGET libzmq-static))
  message(FATAL_ERROR "ZeroMQ version not supported!")
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
    OPTION (ENABLE_DRAFTS "Build and install draft classes and methods" ON)
else ()
    OPTION (ENABLE_DRAFTS "Build and install draft classes and methods" OFF)
endif ()
if (ENABLE_DRAFTS)
    ADD_DEFINITIONS (-DZMQ_BUILD_DRAFT_API)
    set (pkg_config_defines "-DZMQ_BUILD_DRAFT_API=1")
else (ENABLE_DRAFTS)
    set (pkg_config_defines "")
endif (ENABLE_DRAFTS)

message(STATUS "cppzmq v${cppzmq_VERSION}")

set(CPPZMQ_HEADERS
    zmq.hpp
    zmq_addon.hpp
)

foreach (target cppzmq cppzmq-static)
  add_library(${target} INTERFACE)
  target_include_directories(${target} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                                 $<INSTALL_INTERFACE:include>)
endforeach()

target_link_libraries(cppzmq INTERFACE libzmq)
target_link_libraries(cppzmq-static INTERFACE libzmq-static)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS cppzmq cppzmq-static
        EXPORT ${PROJECT_NAME}-targets)

install(FILES ${CPPZMQ_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# GNUInstallDirs "DATADIR" wrong here; CMake search path wants "share".
set(CPPZMQ_CMAKECONFIG_INSTALL_DIR "share/cmake/${PROJECT_NAME}" CACHE STRING "install path for cppzmqConfig.cmake")

configure_file(libzmq-pkg-config/FindZeroMQ.cmake
               libzmq-pkg-config/FindZeroMQ.cmake
               COPYONLY)

export(EXPORT ${PROJECT_NAME}-targets
     FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${CPPZMQ_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/libzmq-pkg-config/FindZeroMQ.cmake
              DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR}/libzmq-pkg-config)

option(CPPZMQ_BUILD_TESTS "Whether or not to build the tests" ON)

if (CPPZMQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
