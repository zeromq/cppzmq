#!/usr/bin/env bash

set -x
set -e

SOURCE=${PWD}
BUILD_TYPE=${BUILD_TYPE:-cmake}
ZMQ_VERSION=${ZMQ_VERSION:-4.2.5}
ENABLE_DRAFTS=${ENABLE_DRAFTS:-OFF}
COVERAGE=${COVERAGE:-OFF}
LIBZMQ=${PWD}/libzmq-build
CPPZMQ=${PWD}/cppzmq-build
# Travis machines have 2 cores
JOBS=2

libzmq_install() {
    curl -L https://github.com/zeromq/libzmq/archive/v"${ZMQ_VERSION}".tar.gz \
      >zeromq.tar.gz
    tar -xvzf zeromq.tar.gz
    if [ "${BUILD_TYPE}" = "cmake" ] ; then
        cmake -Hlibzmq-${ZMQ_VERSION} -B${LIBZMQ} -DWITH_PERF_TOOL=OFF \
                                                  -DZMQ_BUILD_TESTS=OFF \
                                                  -DCMAKE_BUILD_TYPE=Release \
                                                  -DENABLE_DRAFTS=${ENABLE_DRAFTS}
        cmake --build ${LIBZMQ} -- -j${JOBS}
    elif [ "${BUILD_TYPE}" = "pkgconfig" ] ; then
        pushd .
        cd libzmq-${ZMQ_VERSION}
        ./autogen.sh &&
        ./configure --prefix=${LIBZMQ} &&
        make -j${JOBS}
        make install
        popd
    fi
}


# build zeromq first
cppzmq_build() {
    CMAKE_PREFIX_PATH=${LIBZMQ} \
    cmake -H. -B${CPPZMQ} -DENABLE_DRAFTS=${ENABLE_DRAFTS}
    cmake --build ${CPPZMQ} -- -j${JOBS}
}

cppzmq_tests() {
    pushd .
    cd ${CPPZMQ}
    if [ "${COVERAGE}" = "ON" ] ; then
        lcov -c -i -b ${SOURCE} -d . -o Coverage.baseline
    fi
    ctest -V -j${JOBS}
    if [ "${COVERAGE}" = "ON" ] ; then
        lcov -c -b ${SOURCE} -d . -o Coverage.out
        lcov -a Coverage.baseline -a Coverage.out -o Coverage.lcov
    fi
    popd
}

cppzmq_demo() {
    pushd .
    CMAKE_PREFIX_PATH=${LIBZMQ}:${CPPZMQ} \
    cmake -Hdemo -Bdemo/build
    cmake --build demo/build
    cd demo/build
    ctest -V
    popd
}

if [ "${ZMQ_VERSION}" != "" ] ; then libzmq_install ; fi

cppzmq_build
cppzmq_tests
cppzmq_demo
