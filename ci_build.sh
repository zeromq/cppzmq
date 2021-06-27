#!/usr/bin/env bash

set -x
set -e

BUILD_TYPE=${BUILD_TYPE:-cmake}
ZMQ_VERSION=${ZMQ_VERSION:-4.3.1}
ENABLE_DRAFTS=${ENABLE_DRAFTS:-OFF}
COVERAGE=${COVERAGE:-OFF}
LIBZMQ=${PWD}/libzmq-build
CPPZMQ=${PWD}/cppzmq-build
# Travis machines have 2 cores
JOBS=2

cmake_install() {
  local CMAKE_INSTALL_DIR=/tmp/cmake.root
  local CMAKE_SUFFIX="none"

  if [ $TRAVIS_OS_NAME = "linux" ]; then
    CMAKE_SUFFIX="linux-x86_64"
  elif [ $TRAVIS_OS_NAME = "osx" ]; then
    CMAKE_SUFFIX="macos10.10-universal"
  else
    echo "TRAVIS_OS_NAME $TRAVIS_OS_NAME not expected"
    exit 1
  fi

  mkdir -p $CMAKE_INSTALL_DIR
  wget -qO- "https://cmake.org/files/v3.20/cmake-3.20.5-$CMAKE_SUFFIX.tar.gz" \
    | tar --strip-components=1 -xz -C $CMAKE_INSTALL_DIR

  export PATH=$CMAKE_INSTALL_DIR/bin:$PATH
  cmake --version
}

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
    pushd .
    CMAKE_PREFIX_PATH=${LIBZMQ} \
    cmake -H. -B${CPPZMQ} -DENABLE_DRAFTS=${ENABLE_DRAFTS} \
                          -DCOVERAGE=${COVERAGE} \
                          ${CMAKE_CPP_STD:-}
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
    CMAKE_PREFIX_PATH=${LIBZMQ}:${CPPZMQ} \
    cmake -Hdemo -Bdemo/build
    cmake --build demo/build
    cd demo/build
    ctest -V
    popd
}

cmake_install

if [ "${ZMQ_VERSION}" != "" ] ; then libzmq_install ; fi

cppzmq_build
cppzmq_tests
cppzmq_demo
