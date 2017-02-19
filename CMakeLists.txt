cmake_minimum_required(VERSION 2.8.11)

project(radiance)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_STANDARD 11)

include_directories(BTrack/src)

find_package(Qt5Widgets)
find_package(Qt5Quick)
find_package(Qt5Gui)
find_package(PortAudio)
find_package(FFTW)
find_package(SampleRate)

qt5_add_resources(resources/resources.qrc)

add_executable(radiance WIN32 src/main.cpp
    src/VideoNode.cpp
    src/VideoNodeUI.cpp
    src/Effect.cpp
    src/EffectUI.cpp
    src/CrossFader.cpp
    src/CrossFaderUI.cpp
    src/UISettings.cpp
    src/RenderContext.cpp
    src/Output.cpp
    src/OutputUI.cpp
    src/Lux.cpp
    src/Audio.cpp
    src/GraphicalDisplayUI.cpp
    src/Timebase.cpp
    src/liblux/lux.c
    src/liblux/crc.c
    BTrack/src/BTrack.c
    BTrack/src/OnsetDetectionFunction.c
    resources/resources.qrc)

target_link_libraries(radiance Qt5::Widgets Qt5::Quick Qt5::Gui ${PORTAUDIO_LIBRARIES} ${FFTW_LIBRARIES} ${SAMPLERATE_LIBRARIES})
