#!/usr/bin/env bash

set -x
set -e

if [ "$DRAFT" = "1" ] ; then
    # if we enable drafts during the libzmq cmake build, the pkgconfig
    # data should set ZMQ_BUILD_DRAFT_API in dependent builds, but this
    # does not appear to work (TODO)
    export ZEROMQ_CMAKE_FLAGS="-DENABLE_DRAFTS=ON"
fi

install_zeromq() {
    curl -L https://github.com/zeromq/libzmq/archive/v${ZMQ_VERSION}.tar.gz \
      >zeromq.tar.gz
    tar -xvzf zeromq.tar.gz
    cmake -Hlibzmq-${ZMQ_VERSION} -Blibzmq ${ZEROMQ_CMAKE_FLAGS}
    cmake --build libzmq
}

# build zeromq first

if [ "${ZMQ_VERSION}" != "" ] ; then install_zeromq ; fi

# build cppzmq
pushd .
ZeroMQ_DIR=libzmq cmake -H. -Bbuild ${ZEROMQ_CMAKE_FLAGS}
cmake --build build
cd build
ctest -V
popd

# build cppzmq demo
ZeroMQ_DIR=libzmq cppzmq_DIR=build cmake -Hdemo -Bdemo/build
cmake --build demo/build
cd demo/build
ctest
