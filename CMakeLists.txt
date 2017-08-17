cmake_minimum_required(VERSION 3.0.0)
project(cppzmq)

find_package(ZeroMQ QUIET)

# libzmq autotools install: fallback to pkg-config
if(NOT ZeroMQ_FOUND)
    include(${CMAKE_CURRENT_LIST_DIR}/libzmqPkgConfigFallback.cmake)
endif()

if(NOT ZeroMQ_FOUND)
    message(FATAL_ERROR "ZeroMQ was NOT found!")
endif()

if (ZeroMQ_FOUND AND (NOT TARGET libzmq OR NOT TARGET libzmq-static))
  message(FATAL_ERROR "ZeroMQ version not supported!")
endif()

set (${PROJECT_NAME}_VERSION ${ZeroMQ_VERSION})
message(STATUS "cppzmq v${${PROJECT_NAME}_VERSION}")

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

if (NOT CMAKE_VERSION VERSION_LESS 3.0)
  export(EXPORT ${PROJECT_NAME}-targets
         FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
endif()
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              ${CMAKE_SOURCE_DIR}/libzmqPkgConfigFallback.cmake
              DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
