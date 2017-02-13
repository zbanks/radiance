# - Find libsamplerate
# Find the native libsamplerate includes and library
#
#  SAMPLERATE_INCLUDES    - where to find fftw3.h
#  SAMPLERATE_LIBRARIES   - List of libraries when using libsamplerate.
#  SAMPLERATE_FOUND       - True if libsamplerate found.

if (SAMPLERATE_INCLUDES)
  # Already in cache, be silent
  set (SAMPLERATE_FIND_QUIETLY TRUE)
endif (SAMPLERATE_INCLUDES)

find_path (SAMPLERATE_INCLUDES samplerate.h)

find_library (SAMPLERATE_LIBRARIES NAMES samplerate)

# handle the QUIETLY and REQUIRED arguments and set SAMPLERATE_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (SAMPLERATE DEFAULT_MSG SAMPLERATE_LIBRARIES SAMPLERATE_INCLUDES)

mark_as_advanced (SAMPLERATE_LIBRARIES SAMPLERATE_INCLUDES)
