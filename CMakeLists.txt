cmake_minimum_required(VERSION 2.8.12)
project(cppzmq)

find_package(ZeroMQ REQUIRED)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/zmq.hpp" cppzmq_version_defines
    REGEX "#define CPPZMQ_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${cppzmq_version_defines})
    if(ver MATCHES "#define CPPZMQ_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(CPPZMQ_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(${PROJECT_NAME}_VERSION 
    ${CPPZMQ_VERSION_MAJOR}.${CPPZMQ_VERSION_MINOR}.${CPPZMQ_VERSION_PATCH})
message(STATUS "cppzmq v${${PROJECT_NAME}_VERSION}")

set(CPPZMQ_HEADERS
    zmq.hpp
    zmq_addon.hpp
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(FILES ${CPPZMQ_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# GNUInstallDirs "DATADIR" wrong here; CMake search path wants "share".
set(CPPZMQ_CMAKECONFIG_INSTALL_DIR "share/cmake/${PROJECT_NAME}" CACHE STRING "install path for cppzmqConfig.cmake")

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${CPPZMQ_CMAKECONFIG_INSTALL_DIR})
