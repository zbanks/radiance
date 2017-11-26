#!/bin/bash -x

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    cmake -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.9.1/ -DCMAKE_BUILD_TYPE=Release ..
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    cmake -DCMAKE_PREFIX_PATH=/opt/qt59/ -DCMAKE_BUILD_TYPE=Release ..
fi
