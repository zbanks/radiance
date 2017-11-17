#!/bin/bash -x

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    ehco "osx before_script"
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    cmake -DCMAKE_PREFIX_PATH=/opt/qt59/ -DCMAKE_BUILD_TYPE=Release ..
fi
