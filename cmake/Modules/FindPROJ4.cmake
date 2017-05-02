# - Find PROJ4
# Find the PROJ4 includes and library
#
#  PROJ4_INCLUDE_DIR - Where to find PROJ4 includes
#  PROJ4_LIBRARIES   - List of libraries when using PROJ4
#  PROJ4_FOUND       - True if PROJ4 was found

IF(PROJ4_INCLUDE_DIR)
    SET(PROJ4_FIND_QUIETLY TRUE)
ENDIF(PROJ4_INCLUDE_DIR)

FIND_PATH(PROJ4_INCLUDE_DIR "proj_api.h"
          PATHS
          $ENV{EXTERNLIBS}/proj4/include
          ~/Library/Frameworks/include
          /Library/Frameworks/include
          /usr/local/include
          /usr/include
          /sw/include # Fink
          /opt/local/include # DarwinPorts
          /opt/csw/include # Blastwave
          /opt/include)

SET(PROJ4_NAMES Proj4 proj proj_4_8 proj_4_9)

FIND_LIBRARY(PROJ4_LIBRARY NAMES ${PROJ4_NAMES}
             PATHS
             $ENV{EXTERNLIBS}/proj4
             ~/Library/Frameworks
             /Library/Frameworks
             /usr/local
             /usr
             /sw
             /opt/local
             /opt/csw
             /opt
             PATH_SUFFIXES lib lib64)

IF(PROJ4_INCLUDE_DIR)
    FILE(READ ${PROJ4_INCLUDE_DIR}/proj_api.h _proj_api_content)
    
    string(REGEX MATCH "#define PJ_VERSION[ \t]*([0-9]*)\n" _version_match ${_proj_api_content})
    string(SUBSTRING "${CMAKE_MATCH_1}" 0 1 _proj_api_major)
    string(SUBSTRING "${CMAKE_MATCH_1}" 1 1 _proj_api_minor)
    string(SUBSTRING "${CMAKE_MATCH_1}" 2 1 _proj_api_patch)

    IF(_version_major_match AND _version_minor_match AND _version_patch_match)
        SET(PROJ4_VERSION_STRING "${_proj_api_major}.${_proj_api_minor}.${_proj_api_patch}")
    ELSE()
        IF(NOT PROJ4_FIND_QUIETLY)
            MESSAGE(STATUS "Failed to get version information from ${PROJ4_INCLUDE_DIR}/proj_api.h")
        ENDIF()
    ENDIF()
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)

SET(PROJ4_LIBRARIES ${PROJ4_LIBRARY})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROJ4 DEFAULT_MSG PROJ4_LIBRARY PROJ4_INCLUDE_DIR)

MARK_AS_ADVANCED(PROJ4_LIBRARY PROJ4_INCLUDE_DIR)

IF(PROJ4_FOUND)
    SET(PROJ4_INCLUDE_DIRS ${PROJ4_INCLUDE_DIR})
ENDIF(PROJ4_FOUND)
