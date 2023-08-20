# syntax=docker/dockerfile:1
FROM nvidia/opengl:1.0-glvnd-runtime-ubuntu16.04
# FROM ubuntu:xenial-20210804
WORKDIR /app
COPY . .
RUN apt update && apt install software-properties-common -y && \
     apt-add-repository ppa:beineri/opt-qt591-xenial
RUN  apt update && \
     apt-get install git cmake qt59base qt59declarative qt59multimedia qt59quickcontrols \ 
     qt59imageformats qt59graphicaleffects qt59quickcontrols2 qt59script libfftw3-dev libsamplerate0-dev \ 
     libasound2-dev libmpv-dev libdrm-dev libgl1-mesa-dev portaudio19-dev autoconf libtool xvfb alsa-tools \ 
     alsa-utils libsndfile1-dev -y 
RUN git submodule update --init
RUN mkdir build && cd build
RUN git clone https://github.com/thestk/rtmidi            # build & install
RUN cd rtmidi && git checkout 88e53b9 && ./autogen.sh && make && make install
SHELL ["/bin/bash", "-c"]
RUN cd /app/build && cmake .. -DCMAKE_PREFIX_PATH=/opt/qt59/ && make -j$(($(nproc) - 1))
ENTRYPOINT [ "/app/build/radiance"]
