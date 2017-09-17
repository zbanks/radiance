cmake_minimum_required(VERSION 2.8.11)

project(radiance)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DILUT_USE_OPENGL")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O2 -ffast-math -march=native")
set(CMAKE_CXX_STANDARD 14)

include(Doxygen)
include_directories(BTrack/src)

find_package(Qt5Widgets)
find_package(Qt5Quick)
find_package(Qt5Gui)
find_package(PortAudio)
find_package(FFTW)
find_package(SampleRate)
find_package(RtMidi)
find_package(OpenGL)
find_package(DevIL)

qt5_add_resources(resources/resources.qrc)

add_library(libradiance
    src/Audio.cpp
    src/Lux.cpp
    src/Midi.cpp
    src/Output.cpp
    src/Timebase.cpp
    src/UISettings.cpp
    src/NodeRegistry.cpp
    src/VideoNode.cpp
    src/EffectNode.cpp
    src/ImageNode.cpp
    src/FramebufferVideoNodeRender.cpp
    src/GraphicalDisplay.cpp
    src/Model.cpp
    src/OpenGLWorkerContext.cpp
    src/OpenGLWorker.cpp
    src/View.cpp
    src/Chain.cpp
    src/Context.cpp
    src/liblux/lux.c
    src/liblux/crc.c
    BTrack/src/BTrack.c
    BTrack/src/OnsetDetectionFunction.c
    resources/resources.qrc
    )

add_executable(radiance WIN32 src/main.cpp
    src/QQuickVideoNodePreview.cpp
    src/QQuickOutputItem.cpp
    src/QQuickOutputWindow.cpp
    resources/resources.qrc)

add_executable(radiance_cli WIN32 src/main_standalone.cpp)

target_link_libraries(libradiance Qt5::Widgets Qt5::Quick Qt5::Gui ${FFTW_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${RTMIDI_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${OPENGL_LIBRARIES} ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES})
target_link_libraries(radiance libradiance Qt5::Widgets Qt5::Quick Qt5::Gui ${FFTW_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${RTMIDI_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${OPENGL_LIBRARIES} ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES})
target_link_libraries(radiance_cli libradiance Qt5::Widgets Qt5::Quick Qt5::Gui ${FFTW_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${RTMIDI_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${OPENGL_LIBRARIES} ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES})
