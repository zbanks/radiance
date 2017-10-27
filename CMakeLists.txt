cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

project(radiance)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3 -O2 -ffast-math -march=native -DDEBUG_RESOURCES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O2 -ffast-math -march=native -DDEBUG_RESOURCES")
set(CMAKE_CXX_STANDARD 14)

include(Doxygen)
include_directories(BTrack/src)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickControls2 REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(PortAudio REQUIRED)
find_package(FFTW REQUIRED)
find_package(SampleRate REQUIRED)
find_package(OpenGL REQUIRED)
find_package(RtMidi)
find_package(MPV)

set(libradiance_SOURCES)
list(APPEND libradiance_SOURCES
    src/Audio.cpp
    src/Output.cpp
    src/Timebase.cpp
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
    src/OutputImageSequence.cpp
    src/Controls.cpp
    src/BaseVideoNodeTile.cpp
    src/Paths.cpp
    BTrack/src/BTrack.c
    BTrack/src/OnsetDetectionFunction.c
    )

set(radiance_LIBRARIES)
list(APPEND radiance_LIBRARIES
    Qt5::Widgets
    Qt5::Quick
    Qt5::Qml
    Qt5::Gui
    ${FFTW_LIBRARIES}
    ${SAMPLERATE_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    ${OPENGL_LIBRARIES}
    )

include_directories(${FFTW_INCLUDE_DIRS})
include_directories(${SAMPLERATE_INCLUDE_DIRS})
include_directories(${PORTAUDIO_INCLUDE_DIRS})

if(RtMidi_FOUND AND NOT WITHOUT_RTMIDI)
    add_definitions( -DUSE_RTMIDI )
    include_directories(${RTMIDI_INCLUDE_DIRS})
    list(APPEND libradiance_SOURCES src/MidiController.cpp)
    list(APPEND radiance_LIBRARIES ${RTMIDI_LIBRARIES} ${PORTAUDIO_LIBRARIES})
endif()

if(MPV_FOUND AND NOT WITHOUT_MPV)
    add_definitions( -DUSE_MPV )
    include_directories(${MPV_INCLUDE_DIRS})
    list(APPEND libradiance_SOURCES src/MovieNode.cpp)
    list(APPEND radiance_LIBRARIES ${MPV_LIBRARY})
endif()

# lux uses epoll, which is not supported on MacOS
if(NOT APPLE AND NOT WITHOUT_LUX)
    add_definitions( -DUSE_LUX )
    list(APPEND libradiance_SOURCES src/liblux/lux.c
                                    src/liblux/crc.c
                                    src/Lux.cpp
    )
endif()

add_library(libradiance ${libradiance_SOURCES})

add_executable(radiance MACOSX_BUNDLE WIN32 src/main.cpp
    src/QQuickVideoNodePreview.cpp
    src/QQuickOutputItem.cpp
    src/QQuickOutputWindow.cpp)

add_executable(radiance_cli WIN32 src/main_standalone.cpp)

target_link_libraries(libradiance ${radiance_LIBRARIES})
target_link_libraries(radiance libradiance ${radiance_LIBRARIES})
target_link_libraries(radiance_cli libradiance ${radiance_LIBRARIES})

#--------------------------------------------------------------------------------
# Now the installation stuff below
#--------------------------------------------------------------------------------
SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/radiance")
IF(APPLE)
  SET(CMAKE_INSTALL_PREFIX .)
  SET(resources_dest_dir "radiance.app/Contents/Resources")
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/radiance.app")
ENDIF(APPLE)
IF(WIN32)
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/radiance.exe")
ENDIF(WIN32)

#--------------------------------------------------------------------------------
# Install the radiance application, on Apple, the bundle is at the root of the
# install tree, and on other platforms it'll go into the bin directory.
#--------------------------------------------------------------------------------
INSTALL(TARGETS radiance
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
    )

INSTALL(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/"
            DESTINATION
            "${resources_dest_dir}" COMPONENT Runtime)

# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well
# Or just run make package
set(CPACK_BINARY_DRAGNDROP ON)
include(CPack)
