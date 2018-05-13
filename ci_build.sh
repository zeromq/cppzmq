#!/usr/bin/env bash

set -x
set -e

LIBZMQ=${PWD}/libzmq-build
CPPZMQ=${PWD}/cppzmq-build
# Travis machines have 2 cores
JOBS=2

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
    cmake -Hlibzmq-${ZMQ_VERSION} -B${LIBZMQ} -DWITH_PERF_TOOL=OFF \
                                              -DZMQ_BUILD_TESTS=OFF \
                                              -DCMAKE_BUILD_TYPE=Release \
                                              ${ZEROMQ_CMAKE_FLAGS}
    cmake --build ${LIBZMQ} -- -j${JOBS}
}

# build zeromq first

if [ "${ZMQ_VERSION}" != "" ] ; then install_zeromq ; fi

# build cppzmq
pushd .
ZeroMQ_DIR=${LIBZMQ} cmake -H. -B${CPPZMQ} ${ZEROMQ_CMAKE_FLAGS}
cmake --build ${CPPZMQ} -- -j${JOBS}
cd ${CPPZMQ}
ctest -V -j${JOBS}
popd

# build cppzmq demo
ZeroMQ_DIR=${LIBZMQ} cppzmq_DIR=${CPPZMQ} cmake -Hdemo -Bdemo/build
cmake --build demo/build
cd demo/build
ctest -V
