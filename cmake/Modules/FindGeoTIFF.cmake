###############################################################################
#
# CMake module to search for GeoTIFF library
#
# On success, the macro sets the following variables:
# GEOTIFF_FOUND       = if the library found
# GEOTIFF_LIBRARIES   = full path to the library
# GEOTIFF_INCLUDE_DIR = where to find the library headers
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

SET(GEOTIFF_NAMES geotiff)

IF(MSVC)
    SET(GEOTIFF_INCLUDE_DIR "$ENV{LIB_DIR}/include" CACHE STRING INTERNAL)

    SET(GEOTIFF_NAMES ${GEOTIFF_NAMES} geotiff_i)
    FIND_LIBRARY(GEOTIFF_LIBRARIES NAMES
        NAMES ${GEOTIFF_NAMES}
        PATHS
        "$ENV{LIB_DIR}/lib"
        /usr/lib
        C:/msys/local/lib)
ELSE()
    FIND_PATH(GEOTIFF_INCLUDE_DIR
        geotiff.h
        PATH_SUFFIXES geotiff libgeotiff
        PATHS
        /usr/local/include
        /usr/include)

    FIND_LIBRARY(GEOTIFF_LIBRARIES
        NAMES ${GEOTIFF_NAMES}
        PATHS
        /usr/local/lib
        /usr/lib)
ENDIF()



IF(GEOTIFF_FOUND)
    SET(GEOTIFF_LIBRARIES ${GEOTIFF_LIBRARIES})
ENDIF()

# Handle the QUIETLY and REQUIRED arguments and set SPATIALINDEX_FOUND to TRUE
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GEOTIFF DEFAULT_MSG GEOTIFF_LIBRARIES GEOTIFF_INCLUDE_DIR)
