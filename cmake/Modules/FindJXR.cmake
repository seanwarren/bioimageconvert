# Search for jxr headers and libraries
#
# Find the JXR includes and library
# This module defines
#  JXR_INCLUDE_DIR, where to find JXRGlue.h and other headers
#  JXR_LIBRARIES, the libraries needed to use JXR.
#  JXR_VERSION, The value of JXR_VERSION defined in lcms.h
#  JXR_FOUND, If false, do not try to use JXR.
#
# ==========================================
if(MSYS OR MINGW)
    find_path(JXR_INCLUDE_DIR JXRGlue.h
              $ENV{MINGW_PREFIX}/include/jxrlib $ENV{MINGW_PREFIX}/include)
else()
    find_path(JXR_INCLUDE_DIR JXRGlue.h
              /usr/local/include/jxrlib /usr/include/jxrlib)
endif()

FIND_LIBRARY(JXR_LIBRARIES NAMES libjxrglue${CMAKE_SHARED_LIBRARY_SUFFIX} libjxrglue${CMAKE_STATIC_LIBRARY_SUFFIX})


# If everything is ok, set variable for the package
# =================================================
if(JXR_INCLUDE_DIR AND JXR_LIBRARIES)
   set(JXR_FOUND TRUE)
else()
   set(JXR_FOUND FALSE)
endif()


if(NOT JXR_FOUND)
    if(JXR_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find required JXR")
   endif()
endif()

mark_as_advanced(JXR_INCLUDE_DIR JXR_LIBRARIES JXR_VERSION)

