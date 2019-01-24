#!/usr/bin/env bash

set -x
set -e

BUILD_TYPE=${BUILD_TYPE:-cmake}
ZMQ_VERSION=${ZMQ_VERSION:-4.2.5}
ENABLE_DRAFTS=${ENABLE_DRAFTS:-OFF}
COVERAGE=${COVERAGE:-OFF}
LIBZMQ=${PWD}/libzmq-build
CATCH2=${PWD}/catch2-build
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

catch2_install() {
    curl -L "https://github.com/catchorg/Catch2/archive/v${CATCH2_VERSION}.tar.gz" \
      >catch2.tar.gz
    tar -xvzf catch2.tar.gz

    mkdir Catch2-${CATCH2_VERSION}/build
    pushd Catch2-${CATCH2_VERSION}/build
    cmake \
        -DCMAKE_INSTALL_PREFIX=${CATCH2} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCATCH_BUILD_TESTING=OFF \
        -DCATCH_BUILD_EXAMPLES=OFF \
        -DCATCH_BUILD_EXTRA_TESTS=OFF \
	..
    cmake --build . -- -j${JOBS}
    cmake --build . -- install
    popd
}


# build zeromq first
cppzmq_build() {
    pushd .
    CMAKE_PREFIX_PATH=${LIBZMQ}:${CATCH2} \
    cmake -H. -B${CPPZMQ} -DENABLE_DRAFTS=${ENABLE_DRAFTS} \
                          -DCOVERAGE=${COVERAGE}
    cmake --build ${CPPZMQ} -- -j${JOBS}
    popd
}

cppzmq_tests() {
    pushd .
    cd ${CPPZMQ}
    ctest -V -j${JOBS}
    popd
}

cppzmq_demo() {
    pushd .
    CMAKE_PREFIX_PATH=${LIBZMQ}:${CATCH2}:${CPPZMQ} \
    cmake -Hdemo -Bdemo/build
    cmake --build demo/build
    cd demo/build
    ctest -V
    popd
}

if [ "${ZMQ_VERSION}" != "" ] ; then libzmq_install ; fi
if [ "${CATCH2_VERSION}" != "" ] ; then catch2_install ; fi

cppzmq_build
cppzmq_tests
cppzmq_demo
