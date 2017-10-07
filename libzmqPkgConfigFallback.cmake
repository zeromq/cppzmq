find_package(PkgConfig)
pkg_check_modules(PC_LIBZMQ QUIET libzmq)

set(ZeroMQ_VERSION ${PC_LIBZMQ_VERSION})
find_library(ZeroMQ_LIBRARY NAMES libzmq.so libzmq.dylib libzmq.dll
             PATHS ${PC_LIBZMQ_LIBDIR} ${PC_LIBZMQ_LIBRARY_DIRS})
find_library(ZeroMQ_STATIC_LIBRARY NAMES libzmq.a libzmq.dll.a
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
