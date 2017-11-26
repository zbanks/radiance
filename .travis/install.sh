#!/bin/bash -x

if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    brew install cmake
    brew install qt5@5.9.1
    brew install fftw
    brew install libsamplerate
    brew install mpv
    brew install portaudio
    brew install rtmidi
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
    sudo apt-get install -qq g++-6
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 90
    sudo apt-get -qy install doxygen graphviz qt59base qt59multimedia qt59quickcontrols qt59imageformats qt59quickcontrols2 qt59script libfftw3-dev libsamplerate0-dev libasound2-dev libmpv-dev

    git clone --quiet --depth=100 "https://github.com/EddieRingle/portaudio" ~/builds/portaudio \
    && pushd ~/builds/portaudio \
    && ./configure \
    && make \
    && sudo make install \
    && popd

    git clone --quiet --depth=100 "https://github.com/thestk/rtmidi" ~/builds/rtmidi \
    && pushd ~/builds/rtmidi \
    && ./autogen.sh \
    && ./configure \
    && make \
    && sudo make install \
    && popd

    curl -L https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage > linuxdeployqt \
    && chmod +x linuxdeployqt \
    && sudo mv linuxdeployqt /usr/local/bin

    sudo ldconfig
fi
