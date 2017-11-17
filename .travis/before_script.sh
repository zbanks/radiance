#!/bin/bash -x

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then

elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    cmake -DCMAKE_PREFIX_PATH=/opt/qt59/ -DCMAKE_BUILD_TYPE=Release ..
fi
