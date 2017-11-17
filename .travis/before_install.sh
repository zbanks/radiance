#!/bin/bash -x

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then

elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-add-repository -y ppa:beineri/opt-qt591-trusty
    sudo add-apt-repository -y ppa:mc3man/testing6
    sudo apt-get -qy update
fi
