This is C++ binding for 0MQ

The contribution policy is at: http://rfc.zeromq.org/spec:22

Build steps:

1. Build [libzmq](https://github.com/zeromq/libzmq) via cmake. This does an out of source build and installs the build files
   - download and unzip the lib, cd to directory
   - mkdir build
   - cd build
   - cmake ..
   - sudo make -j4 install

2. Build cppzmq via cmake. This does an out of source build and installs the build files
   - download and unzip the lib, cd to directory
   - mkdir build
   - cd build
   - cmake ..
   - sudo make -j4 install

Using this:

A cmake find package scripts is provided for you to easily include this library.
Add these lines in your CMakeLists.txt to include the headers and library files of
cpp zmq (which will also include libzmq for you).

```
#find cppzmq wrapper, installed by make of cppzmq
find_package(cppzmq)
if(cppzmq_FOUND)
    include_directories(${cppzmq_INCLUDE_DIR})
    target_link_libraries(*Your Project Name* ${cppzmq_LIBRARY})
endif()
```


