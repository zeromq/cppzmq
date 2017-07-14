cmake_minimum_required(VERSION 2.8.12)
project(cppzmq)

find_package(ZeroMQ)

# libzmq autotools install: fallback to pkg-config
if(NOT ZeroMQ_FOUND)
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBZMQ QUIET libzmq)

    set(ZeroMQ_VERSION ${PC_LIBZMQ_VERSION})
    find_library(ZeroMQ_LIBRARY NAMES libzmq.so
                 PATHS ${PC_LIBZMQ_LIBDIR} ${PC_LIBZMQ_LIBRARY_DIRS})
    find_library(ZeroMQ_STATIC_LIBRARY NAMES libzmq.a
                 PATHS ${PC_LIBZMQ_LIBDIR} ${PC_LIBZMQ_LIBRARY_DIRS})

    add_library(libzmq SHARED IMPORTED)
    set_property(TARGET libzmq PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PC_LIBZMQ_INCLUDE_DIRS})
    set_property(TARGET libzmq PROPERTY IMPORTED_LOCATION ${ZeroMQ_LIBRARY})

    add_library(libzmq-static STATIC IMPORTED ${PC_LIBZMQ_INCLUDE_DIRS})
    set_property(TARGET libzmq-static PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PC_LIBZMQ_INCLUDE_DIRS})
    set_property(TARGET libzmq-static PROPERTY IMPORTED_LOCATION ${ZeroMQ_STATIC_LIBRARY})

    if(ZeroMQ_LIBRARY AND ZeroMQ_STATIC_LIBRARY)
        set(ZeroMQ_FOUND ON)
    endif()
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
              DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
