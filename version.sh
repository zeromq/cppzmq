#!/bin/sh
#
# This script extracts the 0MQ version from zmq.hpp, which is the master
# location for this information.
#
if [ ! -f zmq.hpp ]; then
    echo "version.sh: error: zmq.hpp does not exist" 1>&2
    exit 1
fi
MAJOR=$(grep '^#define CPPZMQ_VERSION_MAJOR \+[0-9]\+' zmq.hpp)
MINOR=$(grep '^#define CPPZMQ_VERSION_MINOR \+[0-9]\+' zmq.hpp)
PATCH=$(grep '^#define CPPZMQ_VERSION_PATCH \+[0-9]\+' zmq.hpp)
if [ -z "$MAJOR" -o -z "$MINOR" -o -z "$PATCH" ]; then
    echo "version.sh: error: could not extract version from zmq.hpp" 1>&2
    exit 1
fi
MAJOR=$(echo $MAJOR | awk '{ print $3 }')
MINOR=$(echo $MINOR | awk '{ print $3 }')
PATCH=$(echo $PATCH | awk '{ print $3 }')
echo $MAJOR.$MINOR.$PATCH | tr -d '\n\r'

