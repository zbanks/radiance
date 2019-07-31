#!/bin/bash -ex

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    brew install cmake
    brew install qt
    brew install fftw
    brew install libsamplerate
    brew install mpv
    brew install portaudio
    brew install rtmidi
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    sudo apt-get -qy install debhelper cmake qtbase5-dev qtdeclarative5-dev \
        qtquickcontrols2-5-dev libfftw3-dev libsamplerate0-dev portaudio19-dev libmpv-dev librtmidi-dev \
        doxygen devscripts fakeroot graphviz

    curl -L https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage > linuxdeployqt \
    && chmod +x linuxdeployqt \
    && sudo mv linuxdeployqt /usr/local/bin

    sudo ldconfig
fi
