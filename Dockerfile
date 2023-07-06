# syntax=docker/dockerfile:1
FROM ubuntu:xenial-20210804
WORKDIR /app
COPY . .
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \ 
     apt update && apt install software-properties-common -y && \
     apt-add-repository ppa:beineri/opt-qt591-xenial
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \ 
     apt update && \
     apt-get install git cmake qtbase5-dev qtdeclarative5-dev qt59multimedia qt59quickcontrols \ 
     qt59imageformats qt59quickcontrols2 qt59script libfftw3-dev libsamplerate0-dev \ 
     libasound2-dev libmpv-dev libdrm-dev libgl1-mesa-dev portaudio19-dev autoconf libtool xvfb -y 
RUN mkdir build && cd build
RUN git clone https://github.com/thestk/rtmidi            # build & install
RUN cd rtmidi && git checkout 88e53b9 && ./autogen.sh && make && make install
SHELL ["/bin/bash", "-c"]
RUN cd /app/build && cmake .. -DCMAKE_PREFIX_PATH=/opt/qt59/ && make -j8
# RUN rm /usr/lib/x86_64-linux-gnu/libGL.so && ldconfig
# RUN Xvfb :2 -screen 0 1024x768x16 &
ENTRYPOINT [ "/app/build/radiance"]
