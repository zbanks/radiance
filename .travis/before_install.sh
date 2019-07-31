#!/bin/bash -ex

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    echo "osx before_install"
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    sudo apt-get -qy update
fi
