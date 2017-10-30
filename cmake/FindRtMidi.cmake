# Try to find RtMidi library, once done this will define:
#
#  RTMIDI_FOUND - if RtMidi has been found 
#  RTMIDI_INCLUDE_DIR - the RtMidi include directory
#  RTMIDI_LIBRARIES - libraries to link against to use RtMidi

find_path(RTMIDI_INCLUDE_DIR RtMidi.h HINTS ${RTMIDI_DIR} PATH_SUFFIXES rtmidi)
find_library(RTMIDI_LIBRARY NAMES librtmidi.a rtmidi HINTS ${RTMIDI_DIR})

# On Mac OS we have to add framworks RtMidi depends on
if (RTMIDI_LIBRARY AND APPLE)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    set(RTMIDI_DEPENDENCIES ${COREMIDI_LIBRARY} ${COREAUDIO_LIBRARY} ${COREFOUNDATION_LIBRARY})
endif (RTMIDI_LIBRARY AND APPLE)

set(RTMIDI_LIBRARIES ${RTMIDI_LIBRARY} ${RTMIDI_DEPENDENCIES})
list(APPEND RTMIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIR})
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RtMidi DEFAULT_MSG RTMIDI_LIBRARY RTMIDI_INCLUDE_DIR)

mark_as_advanced(RTMIDI_LIBRARY)
