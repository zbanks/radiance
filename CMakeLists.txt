cmake_minimum_required(VERSION 2.8.11)

project(radiance)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O2 -ffast-math -march=native")
set(CMAKE_CXX_STANDARD 14)

include_directories(BTrack/src)

find_package(Qt5Widgets)
find_package(Qt5Quick)
find_package(Qt5Gui)
find_package(PortAudio)
find_package(FFTW)
find_package(SampleRate)
find_package(RtMidi)
find_package(OpenGL)

qt5_add_resources(resources/resources.qrc)

add_executable(radiance WIN32 src/main.cpp
    src/Audio.cpp
    src/CrossFader.cpp
    src/CrossFaderUI.cpp
    src/Effect.cpp
    src/EffectUI.cpp
    src/GraphicalDisplayUI.cpp
    src/Lux.cpp
    src/Midi.cpp
    src/Output.cpp
    src/OutputUI.cpp
    src/RenderContext.cpp
    src/Timebase.cpp
    src/UISettings.cpp
    src/VideoNode.cpp
    src/VideoNodeUI.cpp
    src/liblux/lux.c
    src/liblux/crc.c
    BTrack/src/BTrack.c
    BTrack/src/OnsetDetectionFunction.c
    resources/resources.qrc)

target_link_libraries(radiance Qt5::Widgets Qt5::Quick Qt5::Gui ${FFTW_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${RTMIDI_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${OPENGL_LIBRARIES})
