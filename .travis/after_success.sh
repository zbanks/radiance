#!/bin/bash -ex

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    make bundle
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    if [[ -z "$TRAVIS_TAG" ]]
    then
        VERSION="0~dev1"
    else
        VERSION="$TRAVIS_TAG"
    fi

    deploy/debian.sh . deploy/build "$VERSION" 1

    (cd build && make bundle)
    (cd build && make doxygen-doc)
fi
