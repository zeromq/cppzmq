[![Travis-CI Status](https://img.shields.io/travis/zeromq/cppzmq/master.svg?label=Linux%20|%20OSX)](https://travis-ci.org/zeromq/cppzmq)
[![Appveyor Status](https://img.shields.io/appveyor/ci/zeromq/cppzmq/master.svg?label=Windows)](https://ci.appveyor.com/project/zeromq/cppzmq/branch/master)
[![Coverage Status](https://coveralls.io/repos/github/zeromq/cppzmq/badge.svg?branch=master)](https://coveralls.io/github/zeromq/cppzmq?branch=master)
[![License](https://img.shields.io/github/license/zeromq/cppzmq.svg)](https://github.com/zeromq/cppzmq/blob/master/LICENSE)

Introduction & Design Goals
===========================

cppzmq is a C++ binding for libzmq. It has the following design goals:
 - cppzmq maps the libzmq C API to C++ concepts. In particular:
   - it is type-safe (the libzmq C API exposes various class-like concepts as void*)
   - it provides exception-based error handling (the libzmq C API provides errno-based error handling)
   - it provides RAII-style classes that automate resource management (the libzmq C API requires the user to take care to free resources explicitly)
 - cppzmq is a light-weight, header-only binding. You only need to include the header file zmq.hpp (and maybe zmq_addon.hpp) to use it.
 - zmq.hpp is meant to contain direct mappings of the abstractions provided by the libzmq C API, while zmq_addon.hpp provides additional higher-level abstractions.

There are other C++ bindings for ZeroMQ with different design goals. In particular, none of the following bindings are header-only:
 - [zmqpp](https://github.com/zeromq/zmqpp) is a high-level binding to libzmq.
 - [czmqpp](https://github.com/zeromq/czmqpp) is a binding based on the high-level czmq API.
 - [fbzmq](https://github.com/facebook/fbzmq) is a binding that integrates with Apache Thrift and provides higher-level abstractions in addition. It requires C++14.

Supported platforms
===================

 - Only a subset of the platforms that are supported by libzmq itself are supported. Some features already require a compiler supporting C++11. In the future, probably all features will require C++11. To build and run the tests, cmake and googletest are required.
 - Tested libzmq versions are
   - 4.2.0 (without DRAFT API)
   - 4.2.5 (with and without DRAFT API)
 - Platforms with full support (i.e. CI executing build and tests)
   - Ubuntu 14.04 x64 (with gcc 4.8.4) (without DRAFT API only)
   - Ubuntu 14.04 x64 (with gcc 7.3.0)
   - Visual Studio 2015 x86
   - Visual Studio 2017 x86
   - OSX (with clang xcode9.1) (without DRAFT API)
 - Additional platforms that are known to work:
   - We have no current reports on additional platforms that are known to work yet. Please add your platform here. If CI can be provided for them with a cloud-based CI service working with GitHub, you are invited to add CI, and make it possible to be included in the list above.
 - Additional platforms that probably work:
   - Any platform supported by libzmq that provides a sufficiently recent gcc (4.8.1 or newer) or clang (3.3 or newer)
   - Visual Studio 2012+ x86/x64

Contribution policy
===================

The contribution policy is at: http://rfc.zeromq.org/spec:22

Build instructions
==================

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
target_link_libraries(*Your Project Name* cppzmq)
```


