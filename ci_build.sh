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
    pushd .

    mkdir libzmq
    cd libzmq
    curl -L https://github.com/zeromq/libzmq/archive/v${ZMQ_VERSION}.tar.gz >zeromq.tar.gz
    tar -xvzf zeromq.tar.gz
    cd libzmq-${ZMQ_VERSION}

    mkdir build
    cd build
    cmake .. ${ZEROMQ_CMAKE_FLAGS}
    sudo make -j4 install

    popd
}

# build zeromq first

if [ "${ZMQ_VERSION}" != "" ] ; then install_zeromq ; fi

# build cppzmq

pushd .
mkdir build
cd build
cmake .. ${ZEROMQ_CMAKE_FLAGS}
cmake --build .
sudo make -j4 install
make test ARGS="-V"
popd

# build cppzmq demo
cd demo
mkdir build
cd build
cmake ..
cmake --build .
ctest
