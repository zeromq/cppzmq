#!/usr/bin/env bash

set -x
set -e

if [ "$DRAFT" = "1" ] ; then
    # if we enable drafts during the libzmq cmake build, the pkgconfig
    # data should set ZMQ_BUILD_DRAFT_API in dependent builds, but this
    # does not appear to work (TODO)
    export ZEROMQ_CMAKE_FLAGS="-DENABLE_DRAFTS=ON"
fi

LIBZMQ=${PWD}/libzmq-build
CPPZMQ=${PWD}/cppzmq-build
install_zeromq() {
    curl -L https://github.com/zeromq/libzmq/archive/v${ZMQ_VERSION}.tar.gz \
      >zeromq.tar.gz
    tar -xvzf zeromq.tar.gz
    if [ "${ZMQ_BUILD_TYPE}" = "cmake" ] ; then
        cmake -Hlibzmq-${ZMQ_VERSION} -B${LIBZMQ} -DWITH_PERF_TOOL=OFF \
                                                  -DZMQ_BUILD_TESTS=OFF \
                                                  -DCMAKE_BUILD_TYPE=Release \
                                                  ${ZEROMQ_CMAKE_FLAGS}
        cmake --build ${LIBZMQ}
    elif [ "${ZMQ_BUILD_TYPE}" = "pkgconf" ] ; then
        pushd .
        cd libzmq-${ZMQ_VERSION}
        ./autogen.sh
        ./configure
        sudo make VERBOSE=1 -j5 install
        popd
    else
        echo "Unsupported build type ${ZMQ_BUILD_TYPE}."
        exit 1
    fi
}

# build zeromq first

if [ "${ZMQ_VERSION}" != "" ] ; then install_zeromq ; fi

# build cppzmq
# for pkgconf ZMQ_BUILD_TYPE ZeroMQ_DIR is invalid but it should still work
pushd .
ZeroMQ_DIR=${LIBZMQ} cmake -H. -B${CPPZMQ} ${ZEROMQ_CMAKE_FLAGS}
cmake --build ${CPPZMQ}
cd ${CPPZMQ}
ctest -V
popd

# build cppzmq demo
ZeroMQ_DIR=${LIBZMQ} cppzmq_DIR=${CPPZMQ} cmake -Hdemo -Bdemo/build
cmake --build demo/build
cd demo/build
ctest
