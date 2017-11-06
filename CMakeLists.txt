######################################################################
# libBioImage Project file
# run:
#   cmake - in order to generate Makefile for your platform
#   make  - to compile the library
#   ctest - to run the automatic tests
#
#
# Developed by:  Mario Emmenlauer (mario@emmenlauer.de)
#                Balanstrasse 43, 81669 Munich
#                http://www.biodataanalysis.de/
#
# With contributions by:
#                Dmitry Fedorov ()
#
#
# TODO / NOTES:
# Currently these cmake build instructions are ordered by libraries,
# for historic reasons. So currently the order of instructions is:
#  (1)   library X
#  (1a)  automatic detection of X
#  (1b)  add include paths for internal X
#  (1c)  build instructions for internal X
#  (2)   library Y
#  ...
#  (end) linker commands for *all* libraries
#
# Better would be to shuffle the order such that the above blocks are
# split as outlined here 1-4:
#  (1) automatic detection for *all* libraries
#  (2) add include paths for *all* libraries
#  (3) build instructions for *all* internal libraries
#  (4) linker commands for *all* libraries
# This would be more correct, because then the include paths are always
# shared for all internal libraries. Currently this is not the case! If
# currently library X is listed below Y, then X will see the includes
# of Y, but not vice versa! Recursive dependencies are currently not
# supported at all, and the order of this build file is relevant!
#
######################################################################
#
# default to at least cmake 3.1.0 because older cmake is effort to support:
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

#---------------------------------------------------------------------
# cmake policies define the behavior
#---------------------------------------------------------------------
# Allow libraries to have whitespace before the library name:
if(POLICY CMP0004)
    cmake_policy(SET CMP0004 OLD)
endif()
# Ensure that cmake links against qtmain target on Windows:
if(POLICY CMP0020)
    cmake_policy(SET CMP0020 NEW)
endif()
# Target names with colon are always ALIAS or IMPORTED targets for us:
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()
# Allow to set a version number in project:
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()
# Always link libraries by full path, to avoid linking i.e. the system zlib:
if(POLICY CMP0060)
    cmake_policy(SET CMP0060 NEW)
endif()


#---------------------------------------------------------------------
# Set project information
#---------------------------------------------------------------------
project(bioimage)
set(LIBBIOIMAGE_VERSION_MAJOR 0)
set(LIBBIOIMAGE_VERSION_MINOR 2)
set(LIBBIOIMAGE_VERSION_PATCH 1)
set(LIBBIOIMAGE_VERSION ${LIBBIOIMAGE_VERSION_MAJOR}.${LIBBIOIMAGE_VERSION_MINOR}.${LIBBIOIMAGE_VERSION_PATCH})


# add an uninstall target
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


# add extra modules:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

# show the build messages:
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set that include directories are pre-pendend, not appended:
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)

# Add folder where are supportive functions
set(CMAKE_INCLUDE_CURRENT_DIR ON)


#---------------------------------------------------------------------
# configuration: editable
#---------------------------------------------------------------------

# internal library config. If Qt5 is not available, disable LIBBIOIMAGE as a default:
option(LIBBIOIMAGE                "Enable BioImageConvert implementation (recommended)"                          ON)
option(LIBBIOIMAGE_TRANSFORMS     "Enable BioImageConvert transforms implementation (optional)"                  ON)

# enable or disable optional thirdparty libraries
option(BIC_ENABLE_QT              "Enable BioImageConvert QT support (optional)"                                 ON)
option(BIC_ENABLE_JXRLIB          "Enable BioImageConvert jxr support (optional)"                                ON)
option(BIC_ENABLE_LIBWEBP         "Enable BioImageConvert webp support (optional)"                               ON)
option(BIC_ENABLE_GDCM            "Enable BioImageConvert gdcm support (optional)"                               ON)
option(BIC_ENABLE_FFMPEG          "Enable BioImageConvert ffmpeg support (optional)"                             OFF)
option(BIC_ENABLE_NIFTI           "Enable BioImageConvert NIfTI support (optional)"                              ON)
option(BIC_ENABLE_LIBJPEG_TURBO   "Enable BioImageConvert turbojpeg support instead of standard jpeg (optional)" ON)
option(BIC_ENABLE_OPENCV          "Enable BioImageConvert OpenCV support (optional)"                             ON)
option(BIC_ENABLE_IMGCNV          "Enable BioImageConvert bimread command line program"                          OFF)
option(BIC_ENABLE_OPENMP          "Enable OpenMP parallelization for release builds (optional)"                  OFF)
option(BIC_ENABLE_THREADSAFE      "Enable Thread Safety for parallelization usage (optional)"                    ON)


# use internal or system thirdparty libraries
option(BIC_INTERNAL_LIBTIFF       "Use BioImageConvert internal provided libtiff implementation (recommended)"   ON)
option(BIC_INTERNAL_LIBJPEG       "Use BioImageConvert internal provided jpeg implementation (optional)"         OFF)
option(BIC_INTERNAL_LIBJPEG_TURBO "Use BioImageConvert internal provided turbojpeg implementation (optional)"    OFF)
option(BIC_INTERNAL_LIBPNG        "Use BioImageConvert internal provided png implementation (optional)"          OFF)
option(BIC_INTERNAL_ZLIB          "Use BioImageConvert internal provided zlib implementation (optional)"         OFF)
option(BIC_INTERNAL_BZLIB         "Use BioImageConvert internal provided bzip implementation (optional)"         OFF)
option(BIC_INTERNAL_EXIV2         "Use BioImageConvert internal provided exiv2 implementation (recommended)"     ON)
option(BIC_INTERNAL_EIGEN         "Use BioImageConvert internal provided eigen implementation (optional)"        OFF)
option(BIC_INTERNAL_LIBRAW        "Use BioImageConvert internal provided libraw implementation (optional)"       OFF)
option(BIC_INTERNAL_LIBGEOTIFF    "Use BioImageConvert internal provided geotiff implementation (optional)"      ON)
option(BIC_INTERNAL_PROJ4         "Use BioImageConvert internal provided proj4 implementation (optional)"        OFF)
option(BIC_INTERNAL_PUGIXML       "Use BioImageConvert internal provided pugixml implementation (optional)"      ON)
option(BIC_INTERNAL_OPENJPEG      "Use BioImageConvert internal provided openjpeg implementation (recommended)"  ON)
option(BIC_INTERNAL_JXRLIB        "Use BioImageConvert internal provided jxr implementation (optional)"          OFF)
option(BIC_INTERNAL_LIBWEBP       "Use BioImageConvert internal provided webp implementation (optional)"         OFF)
option(BIC_INTERNAL_LCMS2         "Use BioImageConvert internal provided lcms2 implementation (optional)"        OFF)
option(BIC_INTERNAL_LZMA          "Use BioImageConvert internal provided lzma implementation (optional)"         OFF)
option(BIC_INTERNAL_GDCM          "Use BioImageConvert internal provided gdcm implementation (recommended)"      ON)
option(BIC_INTERNAL_FFMPEG        "Use BioImageConvert internal provided ffmpeg implementation (optional)"       OFF)
option(BIC_INTERNAL_POLE          "Use BioImageConvert internal provided pole implementation (optional)"         ON)
option(BIC_INTERNAL_JZON          "Use BioImageConvert internal provided jzon implementation (optional)"         ON)
option(BIC_INTERNAL_NIFTI         "Use BioImageConvert internal provided NIfTI implementation (optional)"        ON)

# sanity-check the thirdparty library configuration:
if(NOT LIBBIOIMAGE)
    if(LIBBIOIMAGE_TRANSFORMS)
        message("Option LIBBIOIMAGE is disabled, disabling LIBBIOIMAGE_TRANSFORMS.")
        set(LIBBIOIMAGE_TRANSFORMS OFF)
    endif()
endif()
if(NOT LIBBIOIMAGE)
    if(BIC_ENABLE_IMGCNV)
        message("Option LIBBIOIMAGE is disabled, disabling BIC_ENABLE_IMGCNV.")
        set(BIC_ENABLE_IMGCNV OFF)
    endif()
endif()
if(NOT BIC_ENABLE_LIBJPEG_TURBO)
    if(BIC_INTERNAL_LIBJPEG_TURBO)
        message("Option BIC_ENABLE_LIBJPEG_TURBO is disabled, disabling BIC_INTERNAL_LIBJPEG_TURBO.")
        set(BIC_INTERNAL_LIBJPEG_TURBO OFF)
    endif()
endif()
if(BIC_ENABLE_LIBJPEG_TURBO)
    if(BIC_INTERNAL_LIBJPEG)
        message("Option BIC_ENABLE_LIBJPEG_TURBO is disabled, disabling BIC_INTERNAL_LIBJPEG.")
        set(BIC_INTERNAL_LIBJPEG OFF)
    endif()
endif()
if(NOT BIC_ENABLE_GDCM)
    if(BIC_INTERNAL_GDCM)
        message("Option BIC_ENABLE_GDCM is disabled, disabling BIC_INTERNAL_GDCM.")
        set(BIC_INTERNAL_GDCM OFF)
    endif()
endif()
if(NOT BIC_ENABLE_FFMPEG)
    if(BIC_INTERNAL_FFMPEG)
        message("Option BIC_ENABLE_FFMPEG is disabled, disabling BIC_INTERNAL_FFMPEG.")
        set(BIC_INTERNAL_FFMPEG OFF)
    endif()
endif()


#---------------------------------------------------------------------
# required library versions
#---------------------------------------------------------------------
set(BIC_LIBRAW_REQUIRED_VERSION 0.18)
set(BIC_JXRLIB_REQUIRED_VERSION 1.1.6)
set(BIC_LCMS2_REQUIRED_VERSION 2.6)
set(BIC_LIBTIFF_REQUIRED_VERSION 4.0.4)
set(BIC_PROJ4_REQUIRED_VERSION 4.8.0)
set(BIC_LIBWEBP_REQUIRED_VERSION 0.4.3)
set(BIC_EXIV2_REQUIRED_VERSION 0.25)
set(BIC_LIBJPEG_TURBO_REQUIRED_VERSION 1.2.90)
set(BIC_ZLIB_REQUIRED_VERSION 1.2.7)



#---------------------------------------------------------------------
# automatic configuration and compile instructions
#---------------------------------------------------------------------

# Find the Qt library
if(LIBBIOIMAGE AND BIC_ENABLE_QT)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Gui REQUIRED)
    # NOTE: We use the absolute paths to the libraries for pkg-config:
    get_target_property(Qt5Core_LIBRARIES Qt5::Core LOCATION)
    get_target_property(Qt5Gui_LIBRARIES Qt5::Gui LOCATION)
    set(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES})
endif()


# set default CXX flags:
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

# Set required compiler parameters:
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_VERSION VERSION_LESS "3.1")
        set(CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

STRING(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    message("Building in debug mode, enabling debug flags")
    add_definitions(-DDEBUG -D_DEBUG -D_DEBUG_)
endif()

if(APPLE OR UNIX OR MINGW)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.0)
        set(RELEASE_FLAGS ${RELEASE_FLAGS} -O3 -march=corei7-avx -ffast-math)
        set(DEBUG_FLAGS ${DEBUG_FLAGS} -O1 -march=corei7-avx -ffast-math -ggdb)
    else()
        set(RELEASE_FLAGS ${RELEASE_FLAGS} -O3 -mtune=nocona -ffast-math -mmmx -m3dnow -msse -msse2 -msse3)
        set(DEBUG_FLAGS ${DEBUG_FLAGS} -O1 -mtune=nocona -ffast-math -mmmx -m3dnow -msse -msse2 -msse3 -ggdb)
    endif()
endif()
#if(UNIX OR MINGW OR APPLE)
#    if(NOT APPLE)
#        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
#        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
#        set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -ggdb")
#    endif()
#
#    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0")
#    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0")
#    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0")
#endif()

# check if openMP is supported, enable it:
if(BIC_ENABLE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OpenMP_CXX_FLAGS}")

        add_definitions(-DBIM_USE_OPENMP)
    endif()
endif()


if(BUILD_SHARED_LIBS)
    set(CMAKE_LIBRARY_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
    set(CMAKE_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(CMAKE_LIBRARY_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(CMAKE_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()



#---------------------------------------------------------------------
# configuration paths: editable
#---------------------------------------------------------------------
set(BIM_LSRC          ${CMAKE_CURRENT_SOURCE_DIR}/libsrc)
set(BIM_LIBS          ${CMAKE_CURRENT_SOURCE_DIR}/libs)

set(BIM_LIB_TIF       ${BIM_LSRC}/libtiff)
set(BIM_LIB_JPG       ${BIM_LSRC}/libjpeg)
set(BIM_LIB_JPG_TURBO ${BIM_LSRC}/libjpeg-turbo)
set(BIM_LIB_PNG       ${BIM_LSRC}/libpng)
set(BIM_LIB_Z         ${BIM_LSRC}/zlib)
set(BIM_LIB_BZ2       ${BIM_LSRC}/bzip2)
set(BIM_LIB_BIO       ${BIM_LSRC}/libbioimg)

set(BIM_CORE          ${BIM_LIB_BIO}/core_lib)
set(BIM_FMTS          ${BIM_LIB_BIO}/formats)
set(BIM_FMTS_API      ${BIM_LIB_BIO}/formats_api)
set(BIM_TRANSFORMS    ${BIM_LIB_BIO}/transforms)

set(BIM_LIB_FFMPEG    ${BIM_LSRC}/ffmpeg)
set(BIM_FMT_FFMPEG    ${BIM_FMTS}/mpeg)

set(BIM_LIB_EXIV2     ${BIM_LSRC}/exiv2)
set(BIM_LIB_EIGEN     ${BIM_LSRC}/eigen)
set(BIM_LIB_RAW       ${BIM_LSRC}/libraw)
set(BIM_LIB_FFT       ${BIM_LSRC}/libfftw/src)
set(BIM_LIB_GEOTIF    ${BIM_LSRC}/libgeotiff)
set(BIM_LIB_PROJ4     ${BIM_LSRC}/proj4/src)
set(BIM_LIB_PUGIXML   ${BIM_LSRC}/pugixml/src)
set(BIM_LIB_GDCM      ${BIM_LSRC}/gdcm)
set(BIM_FMT_DICOM     ${BIM_FMTS}/dicom)
set(BIM_LIB_OPENJPEG  ${BIM_LSRC}/openjpeg/src)
set(BIM_LIB_JXRLIB    ${BIM_LSRC}/jxrlib)
set(BIM_LIB_LIBWEBP   ${BIM_LSRC}/libwebp)
set(BIM_LIB_LCMS2     ${BIM_LSRC}/lcms2)
set(BIM_LIB_LZMA      ${BIM_LSRC}/liblzma)
set(BIM_LIB_POLE      ${BIM_LSRC}/pole)
set(BIM_LIB_JZON      ${BIM_LSRC}/jzon)
set(BIM_LIB_NIFTI_LIB ${BIM_LSRC}/nifti/niftilib)
set(BIM_LIB_NIFTI_ZNZ ${BIM_LSRC}/nifti/znzlib)
set(BIM_LIB_NIFTI_FSL ${BIM_LSRC}/nifti/fsliolib)

#---------------------------------------------------------------------
# configuration: automatic
#---------------------------------------------------------------------

if(WIN32 AND NOT MINGW)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(MINGW)
    set(BIM_LIBS_PLTFM ${BIM_LIBS}/mingw)
elseif(WIN32)
    set(BIM_LIBS_PLTFM ${BIM_LIBS}/vc2008)
elseif(APPLE)
    set(BIM_LIBS_PLTFM ${BIM_LIBS}/macosx)
elseif(UNIX)
    set(BIM_LIBS_PLTFM ${BIM_LIBS}/linux/$ENV{HOSTTYPE})
else()
    set(BIM_LIBS_PLTFM ${BIM_LIBS}/linux)
endif()


#---------------------------------------------------------------------
# library configuration: automatic
#---------------------------------------------------------------------

if(APPLE AND BIC_INTERNAL_ZLIB)
    message(FATAL_ERROR "Internal static zlib not supported on Apple platform.")
endif()

# some configs first
if(UNIX OR MINGW OR APPLE)
    add_definitions(-DHAVE_UNISTD_H)
endif()
if(WIN32 AND NOT MINGW)
    add_definitions(-DHAVE_IO_H)
endif()


#---------------------------------------------------------------------
# CMake Settings, Macros and Functions
#---------------------------------------------------------------------

# always link libraries from our binary output path first:
if(NOT LIBRARY_OUTPUT_PATH)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
endif()
link_directories(${LIBRARY_OUTPUT_PATH})

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)


# The following macro can be used to add a thirdparty library target,
# which automatically becomes a dependency of bioimage.
macro(internal_add_library NAME SOURCES HEADERS INSTALLHEADERS LINK_LIBRARIES)
    # add the install-headers to the list of all headers
    set(HEADERS ${HEADERS} ${INSTALLHEADERS})

    # set the actual build target:
    add_library(${NAME} ${SOURCES} ${HEADERS})

    # add a dependency on this internal library:
    #if(LIBBIOIMAGE)
    #    add_dependencies(bioimage ${NAME})
    #endif()

    target_compile_options(${NAME} PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")
    target_compile_options(${NAME} PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")

    # shared libraries need PIC
    set_property(TARGET ${NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

    # allow thirdparty libraries to have slightly less strict compiler warnings:
    #target_compile_options(${NAME} PUBLIC -Wno-sign-compare -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-function -Wno-misleading-indentation)

    target_link_libraries(${NAME} ${LINK_LIBRARIES})

    # Install library and headers
    install(TARGETS ${NAME} EXPORT BioImageTargets DESTINATION lib)
    message(STATUS "INSTALLING ${NAME}")
    install(FILES ${INSTALLHEADERS} DESTINATION include)
endmacro()


#---------------------------------------------------------------------
# libbioimage
#---------------------------------------------------------------------

if(LIBBIOIMAGE)
    # platform defines:
    if(WIN32 AND NOT MINGW)
        add_definitions(-DBIM_WIN)
    endif()

    #---------------------------------------------------------------------
    # Eigen (only for libbioimage internally)
    #---------------------------------------------------------------------
    add_definitions(-DBIM_USE_EIGEN)
    if(BIC_INTERNAL_EIGEN)
        include_directories(${BIM_LIB_EIGEN})
    elseif(LIBBIOIMAGE)
        find_package(Eigen3 REQUIRED)
        include_directories(${EIGEN3_INCLUDE_DIR})
    endif()

    # optional defines:
    if(BIC_ENABLE_QT)
        add_definitions(-DBIM_USE_QT)
    endif()

    if(BIC_ENABLE_OPENCV)
        find_package(OpenCV REQUIRED)
        add_definitions(-DBIM_USE_OPENCV)
    endif()

    #add_definitions(-DBIM_USE_ITK)
    #add_definitions(-DBIM_USE_NUMPY)
    # this is automatically set based on BIM_WIN in libsrc/libbioimg/core_lib/xstring.h:
    #add_definitions(-DBIM_USE_CODECVT)
    #add_definitions(-DBIM_USE_IMAGEMANAGER)

    # enable thread-safety if requested:
    if(BIC_ENABLE_THREADSAFE)
        add_definitions(-DBIC_ENABLE_THREADSAFE)
    endif()

    include_directories(${BIM_LIB_BIO})
    include_directories(${BIM_FMTS_API})
    include_directories(${BIM_FMTS})
    include_directories(${BIM_CORE})

    # core
    set(SOURCES ${SOURCES}
        ${BIM_CORE}/xstring.cpp
        ${BIM_CORE}/xtypes.cpp
        ${BIM_CORE}/tag_map.cpp
        ${BIM_CORE}/xpointer.cpp
        ${BIM_CORE}/xconf.cpp)

    set(HEADERS ${HEADERS}
        ${BIM_CORE}/blob_manager.h)

    set(INSTALLHEADERS ${INSTALLHEADERS}
        ${BIM_LIB_BIO}/BioImageCore
        ${BIM_LIB_BIO}/BioImage
        ${BIM_LIB_BIO}/BioImageFormats)

    set(INSTALLHEADERS ${INSTALLHEADERS}
        ${BIM_CORE}/xpointer.h)

    # Formats API
    set(SOURCES ${SOURCES}
        ${BIM_FMTS_API}/bim_img_format_utils.cpp
        ${BIM_FMTS_API}/bim_buffer.cpp
        ${BIM_FMTS_API}/bim_histogram.cpp
        ${BIM_FMTS_API}/bim_metatags.cpp
        ${BIM_FMTS_API}/bim_image.cpp
        ${BIM_FMTS_API}/bim_image_qt.cpp
        ${BIM_FMTS_API}/bim_image_filters.cpp
        ${BIM_FMTS_API}/bim_image_transforms.cpp
        ${BIM_FMTS_API}/bim_image_opencv.cpp
        ${BIM_FMTS_API}/bim_image_pyramid.cpp
        ${BIM_FMTS_API}/bim_image_proxy.cpp
        ${BIM_FMTS_API}/bim_image_stack.cpp
        ${BIM_FMTS_API}/typeize_buffer.cpp)
    if(BIC_ENABLE_QT)
        set(SOURCES ${SOURCES}
            ${BIM_FMTS_API}/bim_image_qt.cpp)
    endif()

    set(INSTALLHEADERS ${INSTALLHEADERS}
        ${BIM_CORE}/tag_map.h
        ${BIM_CORE}/xconf.h
        ${BIM_CORE}/xtypes.h
        ${BIM_CORE}/xstring.h
        ${BIM_FMTS_API}/bim_buffer.h
        ${BIM_FMTS_API}/bim_primitives.h
        ${BIM_FMTS_API}/bim_histogram.h
        ${BIM_FMTS_API}/bim_image.h
        ${BIM_FMTS_API}/bim_image_5d.h
        ${BIM_FMTS_API}/bim_image_pyramid.h
        ${BIM_FMTS_API}/bim_image_stack.h
        ${BIM_FMTS_API}/bim_image_proxy.h
        ${BIM_FMTS_API}/bim_img_format_interface.h
        ${BIM_FMTS_API}/bim_img_format_utils.h
        ${BIM_FMTS_API}/bim_metatags.h
        ${BIM_FMTS_API}/bim_metatags.def.h
        ${BIM_FMTS_API}/resize.h)

    set(HEADERS ${HEADERS}
        ${BIM_FMTS_API}/rotate.h
        ${BIM_FMTS_API}/slic.h
        ${BIM_FMTS_API}/typeize_buffer.h)
    if(BIC_ENABLE_QT)
        set(HEADERS ${HEADERS}
            ${BIM_FMTS_API}/bim_qt_utils.h)
    endif()

    # Formats
    set(SOURCES ${SOURCES}
        ${BIM_FMTS_API}/bim_image_5d.cpp
        ${BIM_FMTS}/bim_format_manager.cpp
        ${BIM_FMTS}/meta_format_manager.cpp
        ${BIM_FMTS}/bim_exiv_parse.cpp
        ${BIM_FMTS}/bim_lcms_parse.cpp
        ${BIM_FMTS}/tiff/bim_tiny_tiff.cpp
        ${BIM_FMTS}/tiff/bim_tiff_format.cpp
        ${BIM_FMTS}/tiff/bim_tiff_format_io.cpp
        ${BIM_FMTS}/tiff/bim_ometiff_format_io.cpp
        ${BIM_FMTS}/tiff/bim_cz_lsm_format_io.cpp
        ${BIM_FMTS}/tiff/bim_fluoview_format_io.cpp
        ${BIM_FMTS}/tiff/bim_psia_format_io.cpp
        ${BIM_FMTS}/tiff/bim_stk_format_io.cpp
        ${BIM_FMTS}/tiff/bim_geotiff_parse.cpp
        ${BIM_FMTS}/tiff/xtiff.c
        ${BIM_FMTS}/tiff/memio.c
        ${BIM_FMTS}/jpeg/bim_jpeg_format.cpp
        ${BIM_FMTS}/biorad_pic/bim_biorad_pic_format.cpp
        ${BIM_FMTS}/bmp/bim_bmp_format.cpp
        ${BIM_FMTS}/png/bim_png_format.cpp
        ${BIM_FMTS}/nanoscope/bim_nanoscope_format.cpp
        ${BIM_FMTS}/raw/bim_raw_format.cpp
        ${BIM_FMTS}/ibw/bim_ibw_format.cpp
        ${BIM_FMTS}/ome/bim_ome_format.cpp
        ${BIM_FMTS}/ole/bim_ole_format.cpp
        ${BIM_FMTS}/ole/bim_oib_format_io.cpp
        ${BIM_FMTS}/ole/bim_zvi_format_io.cpp
        ${BIM_FMTS}/ole/zvi.cpp
        ${BIM_FMTS}/dcraw/bim_dcraw_format.cpp
        ${BIM_FMTS}/mrc/bim_mrc_format.cpp
        ${BIM_FMTS}/jp2/bim_jp2_format.cpp)

    set(INSTALLHEADERS ${INSTALLHEADERS}
        ${BIM_FMTS}/bim_format_manager.h
        ${BIM_FMTS}/meta_format_manager.h)

    set(HEADERS ${HEADERS}
        ${BIM_FMTS}/dcraw/bim_dcraw_format.h
        ${BIM_FMTS}/mrc/bim_mrc_format.h
        ${BIM_FMTS}/bmp/bim_bmp_format.h
        ${BIM_FMTS}/bim_exiv_parse.h
        ${BIM_FMTS}/ole/oib.h
        ${BIM_FMTS}/ole/zvi.h
        ${BIM_FMTS}/ole/bim_ole_format.h
        ${BIM_FMTS}/jpeg/bim_jpeg_format.h
        ${BIM_FMTS}/png/bim_png_format.h
        ${BIM_FMTS}/raw/bim_raw_format.h
        ${BIM_FMTS}/tiff/bim_ometiff_format.h
        ${BIM_FMTS}/tiff/round.h
        ${BIM_FMTS}/tiff/bim_tiff_format.h
        ${BIM_FMTS}/tiff/bim_tiny_tiff.h
        ${BIM_FMTS}/tiff/bim_stk_format.h
        ${BIM_FMTS}/tiff/memio.h
        ${BIM_FMTS}/tiff/bim_fluoview_format.h
        ${BIM_FMTS}/tiff/bim_psia_format.h
        ${BIM_FMTS}/tiff/bim_cz_lsm_format.h
        ${BIM_FMTS}/ibw/bim_ibw_format.h
        ${BIM_FMTS}/biorad_pic/bim_biorad_pic_format.h
        ${BIM_FMTS}/nanoscope/bim_nanoscope_format.h
        ${BIM_FMTS}/mpeg/parse.h
        ${BIM_FMTS}/mpeg/FfmpegCommon.h
        ${BIM_FMTS}/mpeg/IVideo.h
        ${BIM_FMTS}/mpeg/bim_ffmpeg_format.h
        ${BIM_FMTS}/mpeg/FfmpegIVideo.h
        ${BIM_FMTS}/mpeg/registry.h
        ${BIM_FMTS}/mpeg/matarray.h
        ${BIM_FMTS}/mpeg/FfmpegOVideo.h
        ${BIM_FMTS}/mpeg/debug.h
        ${BIM_FMTS}/mpeg/handle.h
        ${BIM_FMTS}/mpeg/OVideo.h
        ${BIM_FMTS}/ome/bim_ome_format.h
        ${BIM_FMTS}/jp2/bim_jp2_format.h)


    #---------------------------------------------------------------------
    # Transforms
    #---------------------------------------------------------------------

    if(LIBBIOIMAGE_TRANSFORMS)
        add_definitions(-DBIM_USE_FILTERS)
        add_definitions(-DBIM_USE_TRANSFORMS)

        find_package(FFTW REQUIRED)
        include_directories(${FFTW_INCLUDE_DIR})
        
        set(SOURCES ${SOURCES}
            ${BIM_TRANSFORMS}/chebyshev.cpp
            ${BIM_TRANSFORMS}/FuzzyCalc.cpp
            ${BIM_TRANSFORMS}/radon.cpp
            ${BIM_TRANSFORMS}/wavelet/Common.cpp
            ${BIM_TRANSFORMS}/wavelet/convolution.cpp
            ${BIM_TRANSFORMS}/wavelet/DataGrid2D.cpp
            ${BIM_TRANSFORMS}/wavelet/DataGrid3D.cpp
            ${BIM_TRANSFORMS}/wavelet/Filter.cpp
            ${BIM_TRANSFORMS}/wavelet/FilterSet.cpp
            ${BIM_TRANSFORMS}/wavelet/Symlet5.cpp
            ${BIM_TRANSFORMS}/wavelet/Wavelet.cpp
            ${BIM_TRANSFORMS}/wavelet/WaveletHigh.cpp
            ${BIM_TRANSFORMS}/wavelet/WaveletLow.cpp
            ${BIM_TRANSFORMS}/wavelet/WaveletMedium.cpp
            ${BIM_TRANSFORMS}/wavelet/wt.cpp)

        set(HEADERS ${HEADERS}
            ${BIM_TRANSFORMS}/wavelet/DataGrid.h
            ${BIM_TRANSFORMS}/wavelet/wt.h
            ${BIM_TRANSFORMS}/wavelet/DataGrid3D.h
            ${BIM_TRANSFORMS}/wavelet/WaveletLow.h
            ${BIM_TRANSFORMS}/wavelet/Common.h
            ${BIM_TRANSFORMS}/wavelet/convolution.h
            ${BIM_TRANSFORMS}/wavelet/WaveletHigh.h
            ${BIM_TRANSFORMS}/wavelet/FilterSet.h
            ${BIM_TRANSFORMS}/wavelet/DataGrid2D.h
            ${BIM_TRANSFORMS}/wavelet/Symlet5.h
            ${BIM_TRANSFORMS}/wavelet/Filter.h
            ${BIM_TRANSFORMS}/wavelet/Wavelet.h
            ${BIM_TRANSFORMS}/wavelet/WaveletMedium.h
            ${BIM_TRANSFORMS}/FuzzyCalc.h
            ${BIM_TRANSFORMS}/chebyshev.h
            ${BIM_TRANSFORMS}/radon.h)
    endif()


    # optional sources depending on certain libraries:
    if(BIC_ENABLE_NIFTI)
        add_definitions(-DBIM_NIFTI_FORMAT)
        set(SOURCES ${SOURCES}
            ${BIM_FMTS}/nifti/bim_nifti_format.cpp)
    endif()
    if(BIC_ENABLE_FFMPEG)
        add_definitions(-DBIM_FFMPEG_FORMAT)
        set(SOURCES ${SOURCES}
            ${BIM_FMT_FFMPEG}/debug.cpp
            ${BIM_FMT_FFMPEG}/bim_ffmpeg_format.cpp
            ${BIM_FMT_FFMPEG}/FfmpegCommon.cpp
            ${BIM_FMT_FFMPEG}/FfmpegIVideo.cpp
            ${BIM_FMT_FFMPEG}/FfmpegOVideo.cpp
            ${BIM_FMT_FFMPEG}/registry.cpp)
    endif()
    if(BIC_ENABLE_GDCM)
        add_definitions(-DBIM_GDCM_FORMAT)
        set(SOURCES ${SOURCES}
            ${BIM_FMT_DICOM}/bim_dicom_format.cpp)
    endif()
    if(BIC_ENABLE_JXRLIB)
        add_definitions(-DBIM_JXRLIB_FORMAT)
        set(SOURCES ${SOURCES}
            ${BIM_FMTS}/jxr/bim_jxr_format.cpp)
    endif()
    if(BIC_ENABLE_LIBWEBP)
        add_definitions(-DBIM_LIBWEBP_FORMAT)
        set(SOURCES ${SOURCES}
            ${BIM_FMTS}/webp/bim_webp_format.cpp)
    endif()


    # add the install-headers to the list of all headers
    set(HEADERS ${HEADERS} ${INSTALLHEADERS})

    # set the actual build target:
    add_library(bioimage ${SOURCES} ${HEADERS})

    set_target_properties(bioimage PROPERTIES VERSION ${LIBBIOIMAGE_VERSION} SOVERSION ${LIBBIOIMAGE_VERSION_MAJOR})

    #target_compile_options(bioimage PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")
    #target_compile_options(bioimage PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")

    # shared libraries need PIC:
    set_property(TARGET bioimage PROPERTY POSITION_INDEPENDENT_CODE ON)

    if(BIC_ENABLE_QT)
        # NOTE: We use this call to add linker libraries andinclude paths etc.
        #       This is used independent of the variable QT_LIBRARIES, which is
        #       mostly relevant for the pkg-config search path.
        target_link_libraries(bioimage Qt5::Core Qt5::Gui)
    endif()

    # Install library and headers
    install(TARGETS bioimage EXPORT BioImageTargets DESTINATION lib)
    install(FILES ${INSTALLHEADERS} DESTINATION include)
endif()


#---------------------------------------------------------------------
# zlib
#---------------------------------------------------------------------

if(BIC_INTERNAL_ZLIB STREQUAL "AUTO")
    find_package(ZLIB ${BIC_ZLIB_REQUIRED_VERSION})
    if(ZLIB_FOUND)
        set(BIC_INTERNAL_ZLIB OFF)
        message("System ZLib found in suitable version, disabling internal ZLib.")
    else()
        set(BIC_INTERNAL_ZLIB ON)
        message("System ZLib not found (or found unsuitable version), enabling internal ZLib.")
    endif()
endif()
if(BIC_INTERNAL_ZLIB)
    include_directories(${BIM_LIB_Z})

    set(BIC_INTERNAL_ZLIB_SOURCES ${BIC_INTERNAL_ZLIB_SOURCES}
        ${BIM_LIB_Z}/adler32.c
        ${BIM_LIB_Z}/compress.c
        ${BIM_LIB_Z}/crc32.c
        ${BIM_LIB_Z}/deflate.c
        ${BIM_LIB_Z}/infback.c
        ${BIM_LIB_Z}/inffast.c
        ${BIM_LIB_Z}/inflate.c
        ${BIM_LIB_Z}/inftrees.c
        ${BIM_LIB_Z}/trees.c
        ${BIM_LIB_Z}/uncompr.c
        ${BIM_LIB_Z}/zutil.c)

    internal_add_library(z "${BIC_INTERNAL_ZLIB_SOURCES}" "${BIC_INTERNAL_ZLIB_HEADERS}" "${BIC_INTERNAL_ZLIB_INSTALLHEADERS}" "${BIC_INTERNAL_ZLIB_LINK_LIBRARIES}")

    set(ZLIB_LIBRARIES z)
elseif(LIBBIOIMAGE)
    find_package(ZLIB ${BIC_ZLIB_REQUIRED_VERSION} REQUIRED)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()


#---------------------------------------------------------------------
# bzlib
#---------------------------------------------------------------------

if(BIC_INTERNAL_BZLIB)
    include_directories(${BIM_LIB_BZ2})
    set(BIC_INTERNAL_BZLIB_SOURCES ${BIC_INTERNAL_BZLIB_SOURCES}
        ${BIM_LIB_BZ2}/blocksort.c
        ${BIM_LIB_BZ2}/bzlib.c
        ${BIM_LIB_BZ2}/randtable.c
        ${BIM_LIB_BZ2}/compress.c
        ${BIM_LIB_BZ2}/crctable.c
        ${BIM_LIB_BZ2}/decompress.c
        ${BIM_LIB_BZ2}/huffman.c)

    internal_add_library(bz2 "${BIC_INTERNAL_BZLIB_SOURCES}" "${BIC_INTERNAL_BZLIB_HEADERS}" "${BIC_INTERNAL_BZLIB_INSTALLHEADERS}" "${BIC_INTERNAL_BZLIB_LINK_LIBRARIES}")

    set(BZIP2_LIBRARIES bz2)
elseif(LIBBIOIMAGE)
    find_package(BZip2 REQUIRED)
    include_directories(${BZIP2_INCLUDE_DIR})
endif()


#---------------------------------------------------------------------
# lcms2
#---------------------------------------------------------------------

if(BIC_INTERNAL_LCMS2 STREQUAL "AUTO")
    find_package(LCMS2 ${BIC_LCMS2_REQUIRED_VERSION})
    if(LCMS2_FOUND)
        set(BIC_INTERNAL_LCMS2 OFF)
        message("System lcms2 found in suitable version, disabling internal lcms2.")
    else()
        set(BIC_INTERNAL_LCMS2 ON)
        message("System lcms2 not found (or found unsuitable version), enabling internal lcms2.")
    endif()
endif()
if(BIC_INTERNAL_LCMS2)
    include_directories(lcms2 ${BIM_LIB_LCMS2}/include)
    set(BIC_INTERNAL_LCMS2_SOURCES ${BIC_INTERNAL_LCMS2_SOURCES}
        ${BIM_LIB_LCMS2}/src/cmscam02.c
        ${BIM_LIB_LCMS2}/src/cmscgats.c
        ${BIM_LIB_LCMS2}/src/cmscnvrt.c
        ${BIM_LIB_LCMS2}/src/cmserr.c
        ${BIM_LIB_LCMS2}/src/cmsgamma.c
        ${BIM_LIB_LCMS2}/src/cmsgmt.c
        ${BIM_LIB_LCMS2}/src/cmshalf.c
        ${BIM_LIB_LCMS2}/src/cmsintrp.c
        ${BIM_LIB_LCMS2}/src/cmsio0.c
        ${BIM_LIB_LCMS2}/src/cmsio1.c
        ${BIM_LIB_LCMS2}/src/cmslut.c
        ${BIM_LIB_LCMS2}/src/cmsmd5.c
        ${BIM_LIB_LCMS2}/src/cmsmtrx.c
        ${BIM_LIB_LCMS2}/src/cmsnamed.c
        ${BIM_LIB_LCMS2}/src/cmsopt.c
        ${BIM_LIB_LCMS2}/src/cmspack.c
        ${BIM_LIB_LCMS2}/src/cmspcs.c
        ${BIM_LIB_LCMS2}/src/cmsplugin.c
        ${BIM_LIB_LCMS2}/src/cmsps2.c
        ${BIM_LIB_LCMS2}/src/cmssamp.c
        ${BIM_LIB_LCMS2}/src/cmssm.c
        ${BIM_LIB_LCMS2}/src/cmstypes.c
        ${BIM_LIB_LCMS2}/src/cmsvirt.c
        ${BIM_LIB_LCMS2}/src/cmswtpnt.c
        ${BIM_LIB_LCMS2}/src/cmsxform.c)

    set(BIC_INTERNAL_LCMS2_INSTALLHEADERS ${BIC_INTERNAL_LCMS2_INSTALLHEADERS}
        ${BIM_LIB_LCMS2}/include/lcms2.h
        ${BIM_LIB_LCMS2}/include/lcms2_plugin.h)

    internal_add_library(lcms2 "${BIC_INTERNAL_LCMS2_SOURCES}" "${BIC_INTERNAL_LCMS2_HEADERS}" "${BIC_INTERNAL_LCMS2_INSTALLHEADERS}" "${BIC_INTERNAL_LCMS2_LINK_LIBRARIES}")

    set(LCMS2_LIBRARIES lcms2)
elseif(LIBBIOIMAGE)
    find_package(LCMS2 ${BIC_LCMS2_REQUIRED_VERSION} REQUIRED)
    include_directories(${LCMS2_INCLUDE_DIR})
endif()


#---------------------------------------------------------------------
# LZMA - XZ Utils
#---------------------------------------------------------------------

if(BIC_INTERNAL_LZMA)
    include_directories(${BIM_LIB_LZMA}/liblzma/api)
    message(FATAL_ERROR "Internal lzma currently not implemented.")
elseif(LIBBIOIMAGE)
    find_package(LibLZMA REQUIRED)
    include_directories(${LIBLZMA_INCLUDE_DIRS})
endif()


#---------------------------------------------------------------------
# Pole
#---------------------------------------------------------------------

if(BIC_INTERNAL_POLE)
    include_directories(${BIM_LIB_POLE})
    set(BIC_INTERNAL_POLE_SOURCES ${BIC_INTERNAL_POLE_SOURCES}
        ${BIM_LIB_POLE}/pole.cpp)

    internal_add_library(pole "${BIC_INTERNAL_POLE_SOURCES}" "${BIC_INTERNAL_POLE_HEADERS}" "${BIC_INTERNAL_POLE_INSTALLHEADERS}" "${BIC_INTERNAL_POLE_LINK_LIBRARIES}")

    # TODO FIXME: merge this linker command with the automatic system library detection below:
    set(POLE_LIBRARIES pole)
elseif(LIBBIOIMAGE)
    message(FATAL_ERROR "Automatic detection of system pole library not implemented")
endif()


#---------------------------------------------------------------------
# Jzon
#---------------------------------------------------------------------

if(BIC_INTERNAL_JZON)
    include_directories(${BIM_LIB_JZON})
    set(BIC_INTERNAL_JZON_SOURCES ${BIC_INTERNAL_JZON_SOURCES}
        ${BIM_LIB_JZON}/Jzon.cpp)

    internal_add_library(jzon "${BIC_INTERNAL_JZON_SOURCES}" "${BIC_INTERNAL_JZON_HEADERS}" "${BIC_INTERNAL_JZON_INSTALLHEADERS}" "${BIC_INTERNAL_JZON_LINK_LIBRARIES}")

    set(JZON_LIBRARIES jzon)
elseif(LIBBIOIMAGE)
    message(FATAL_ERROR "Automatic detection of system jzon library not implemented")
endif()


#---------------------------------------------------------------------
# Pugixml
#---------------------------------------------------------------------

if(BIC_INTERNAL_PUGIXML)
    include_directories(${BIM_LIB_PUGIXML})
    set(BIC_INTERNAL_PUGIXML_SOURCES ${BIC_INTERNAL_PUGIXML_SOURCES}
        ${BIM_LIB_PUGIXML}/pugixml.cpp)

    internal_add_library(pugixml "${BIC_INTERNAL_PUGIXML_SOURCES}" "${BIC_INTERNAL_PUGIXML_HEADERS}" "${BIC_INTERNAL_PUGIXML_INSTALLHEADERS}" "${BIC_INTERNAL_PUGIXML_LINK_LIBRARIES}")

    set(PUGIXML_LIBRARIES pugixml)
elseif(LIBBIOIMAGE)
    message(FATAL_ERROR "Automatic detection of system pugixml library not implemented")
endif()


#---------------------------------------------------------------------
# NifTI
#---------------------------------------------------------------------

if(BIC_ENABLE_NIFTI)
    if(BIC_INTERNAL_NIFTI)
        include_directories(${BIM_LIB_NIFTI_LIB})
        include_directories(${BIM_LIB_NIFTI_ZNZ})
        include_directories(${BIM_LIB_NIFTI_FSL})

        # niftilib
        set(BIC_INTERNAL_NIFTI_SOURCES ${BIC_INTERNAL_NIFTI_SOURCES}
            ${BIM_LIB_NIFTI_LIB}/nifti1_io.c)
        set(BIC_INTERNAL_NIFTI_INSTALLHEADERS ${BIC_INTERNAL_NIFTI_INSTALLHEADERS}
            ${BIM_LIB_NIFTI_LIB}/nifti1_io.h
            ${BIM_LIB_NIFTI_LIB}/nifti1.h)

        # znzlib
        set(BIC_INTERNAL_NIFTI_SOURCES ${BIC_INTERNAL_NIFTI_SOURCES}
            ${BIM_LIB_NIFTI_ZNZ}/znzlib.c)
        set(BIC_INTERNAL_NIFTI_INSTALLHEADERS ${BIC_INTERNAL_NIFTI_INSTALLHEADERS}
            ${BIM_LIB_NIFTI_ZNZ}/znzlib.h)

        # fsliolib
        set(BIC_INTERNAL_NIFTI_SOURCES ${BIC_INTERNAL_NIFTI_SOURCES}
            ${BIM_LIB_NIFTI_FSL}/fslio.c)
        set(BIC_INTERNAL_NIFTI_INSTALLHEADERS ${BIC_INTERNAL_NIFTI_INSTALLHEADERS}
            ${BIM_LIB_NIFTI_FSL}/fslio.h
            ${BIM_LIB_NIFTI_FSL}/dbh.h)

        internal_add_library(nifti "${BIC_INTERNAL_NIFTI_SOURCES}" "${BIC_INTERNAL_NIFTI_HEADERS}" "${BIC_INTERNAL_NIFTI_INSTALLHEADERS}" "${BIC_INTERNAL_NIFTI_LINK_LIBRARIES}")

        set(NIFTI_LIBRARIES nifti)
    elseif(LIBBIOIMAGE)
        find_package(NIFTI REQUIRED)
        include_directories(${NIFTI_INCLUDE_DIR})
    endif()
endif()


#---------------------------------------------------------------------
# ffmpeg
#---------------------------------------------------------------------

if(BIC_ENABLE_FFMPEG)
    if(BIC_INTERNAL_FFMPEG)
        add_definitions(-DFFMPEG_VIDEO_DISABLE_MATLAB -D__STDC_CONSTANT_MACROS)
        include_directories(${BIM_LIB_FFMPEG}/include)
        if(WIN32 AND NOT MINGW)
            include_directories(${BIM_LIB_FFMPEG}/include-win)
        endif()

        if(WIN32)
            if(NOT EXISTS ${BIM_LIBS_PLTFM}/avformat.lib)
                message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
            endif()
            set(FFMPEG_LIBRARIES
                ${BIM_LIBS_PLTFM}/avcodec.lib
                ${BIM_LIBS_PLTFM}/avformat.lib
                ${BIM_LIBS_PLTFM}/avutil.lib
                ${BIM_LIBS_PLTFM}/swscale.lib)
        elseif(APPLE)
            if(NOT EXISTS ${BIM_LIBS_PLTFM}/libavformat.a)
                message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
            endif()
            set(FFMPEG_LIBRARIES
                ${BIM_LIBS_PLTFM}/libavformat.a
                ${BIM_LIBS_PLTFM}/libavcodec.a
                ${BIM_LIBS_PLTFM}/libswscale.a
                ${BIM_LIBS_PLTFM}/libavutil.a
                ${BIM_LIBS_PLTFM}/libvpx.a
                ${BIM_LIBS_PLTFM}/libx264.a
                ${BIM_LIBS_PLTFM}/libx264.a
                ${BIM_LIBS_PLTFM}/libvpx.a
                ${BIM_LIBS_PLTFM}/libxvidcore.a
                ${BIM_LIBS_PLTFM}/libogg.a
                ${BIM_LIBS_PLTFM}/libtheora.a
                ${BIM_LIBS_PLTFM}/libtheoraenc.a
                ${BIM_LIBS_PLTFM}/libtheoradec.a
                -framework CoreFoundation
                -framework VideoDecodeAcceleration
                -framework QuartzCore)
        elseif(UNIX)
            if(NOT EXISTS ${BIM_LIBS_PLTFM}/libavformat.a)
                message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
            endif()
            set(FFMPEG_LIBRARIES
                ${BIM_LIBS_PLTFM}/libavformat.a
                ${BIM_LIBS_PLTFM}/libavcodec.a
                ${BIM_LIBS_PLTFM}/libswscale.a
                ${BIM_LIBS_PLTFM}/libavutil.a
                ${BIM_LIBS_PLTFM}/libvpx.a
                ${BIM_LIBS_PLTFM}/libx264.a
                -lxvidcore
                -lopenjpeg
                -lschroedinger-1.0
                -ltheora
                -ltheoraenc
                -ltheoradec
                -lpthread
                -lbz2)
        endif()
    elseif(LIBBIOIMAGE)
        find_package(FFMPEG REQUIRED)
        include_directories(${FFMPEG_INCLUDE_DIR})
    endif()
endif()


#---------------------------------------------------------------------
# GDCM - under linux we only use system dynamic version right now
#---------------------------------------------------------------------

if(BIC_ENABLE_GDCM)
    if(BIC_INTERNAL_GDCM)
        add_definitions(-DOPJ_STATIC)
        if(UNIX OR MINGW)
            include_directories(${BIM_LIB_GDCM}/projects/unix)
        else()
            include_directories(${BIM_LIB_GDCM}/projects/win64)
        endif()

        include_directories(${BIM_LIB_GDCM}/Source/Common)
        include_directories(${BIM_LIB_GDCM}/Source/DataDictionary)
        include_directories(${BIM_LIB_GDCM}/Source/DataStructureAndEncodingDefinition)
        include_directories(${BIM_LIB_GDCM}/Source/InformationObjectDefinition)
        include_directories(${BIM_LIB_GDCM}/Source/MediaStorageAndFileFormat)

        if(WIN32)
            if(NOT EXISTS ${BIM_LIBS_PLTFM}/gdcm/gdcmcharls.lib)
                message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
            endif()
            set(GDCM_LIBRARIES
                ${BIM_LIBS_PLTFM}/gdcm/gdcmcharls.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmCommon.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmDICT.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmDSED.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmexpat.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmgetopt.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmIOD.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmjpeg12.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmjpeg16.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmjpeg8.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmMEXD.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmMSFF.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmopenjpeg.lib    
                ${BIM_LIBS_PLTFM}/gdcm/gdcmzlib.lib    
                ${BIM_LIBS_PLTFM}/gdcm/socketxx.lib)
        else()
            if(NOT EXISTS ${BIM_LIBS_PLTFM}/gdcm/libgdcmcharls.a)
                message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
            endif()
            set(GDCM_LIBRARIES
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmcharls.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmCommon.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmDICT.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmDSED.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmexpat.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmIOD.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmjpeg12.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmjpeg16.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmjpeg8.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmMSFF.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmopenjpeg.a    
                ${BIM_LIBS_PLTFM}/gdcm/libgdcmzlib.a)
        endif()
    elseif(LIBBIOIMAGE)
        #find_package(gdcm REQUIRED)
        #include_directories(/usr/include/gdcm-2.6)
        message(FATAL_ERROR "Automatic detection of system gdcm library not implemented")
    endif()
endif()


#---------------------------------------------------------------------
# OPENJPEG
#---------------------------------------------------------------------

if(BIC_INTERNAL_OPENJPEG)
    add_definitions(-DOPJ_HAVE_LIBLCMS2)
    include_directories(${BIM_LIB_OPENJPEG}/lib)
    include_directories(${BIM_LIB_OPENJPEG}/lib/openjp2)
    include_directories(${BIM_LIB_OPENJPEG}/bin)
    include_directories(${BIM_LIB_OPENJPEG}/bin/common)
    include_directories(${BIM_LIB_OPENJPEG}/../project/qt)

    set(BIC_INTERNAL_OPENJPEG_SOURCES ${BIC_INTERNAL_OPENJPEG_SOURCES}
        ${BIM_LIB_OPENJPEG}/lib/openjp2/bio.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/cio.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/dwt.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/event.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/invert.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/image.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/j2k.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/jp2.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/mct.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/mqc.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/openjpeg.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/opj_clock.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/pi.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/raw.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/t1.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/t2.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/tcd.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/tgt.c
        ${BIM_LIB_OPENJPEG}/lib/openjp2/function_list.c)

    set(BIC_INTERNAL_OPENJPEG_INSTALLHEADERS ${BIC_INTERNAL_OPENJPEG_INSTALLHEADERS}
        ${BIM_LIB_OPENJPEG}/lib/openjp2/openjpeg.h)

    set(BIC_INTERNAL_OPENJPEG_HEADERS ${BIC_INTERNAL_OPENJPEG_HEADERS}
        ${BIM_LIB_OPENJPEG}/lib/openjp2/t2.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/raw.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/dwt.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/event.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/mct.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/t1.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/indexbox_manager.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/mqc.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/opj_includes.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/tcd.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/t1_luts.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/opj_malloc.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/cio.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/pi.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/jp2.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/tgt.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/cidx_manager.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/j2k.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/bio.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/image.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/opj_config_private.h
        ${BIM_LIB_OPENJPEG}/lib/openjp2/opj_config.h)

    internal_add_library(openjpeg "${BIC_INTERNAL_OPENJPEG_SOURCES}" "${BIC_INTERNAL_OPENJPEG_HEADERS}" "${BIC_INTERNAL_OPENJPEG_INSTALLHEADERS}" "${BIC_INTERNAL_OPENJPEG_LINK_LIBRARIES}")

    set(OPENJPEG_LIBRARIES openjpeg)
elseif(LIBBIOIMAGE)
    message(FATAL_ERROR "Automatic detection of system openjpeg library not implemented")
endif()


#---------------------------------------------------------------------
# libjpeg-turbo
#---------------------------------------------------------------------

if(BIC_ENABLE_LIBJPEG_TURBO)
    if(BIC_INTERNAL_LIBJPEG_TURBO STREQUAL "AUTO")
        find_package(LibJpegTurbo ${BIC_LIBJPEG_TURBO_REQUIRED_VERSION})
        if(LibJpegTurbo_FOUND)
            set(BIC_INTERNAL_LIBJPEG_TURBO OFF)
            message("System jpeg-turbo found in suitable version, disabling internal jpeg-turbo.")
        else()
            set(BIC_INTERNAL_LIBJPEG_TURBO ON)
            message("System jpeg-turbo not found (or found unsuitable version), enabling internal jpeg-turbo.")
        endif()
    endif()
    if(BIC_INTERNAL_LIBJPEG_TURBO)
        include_directories(${BIM_LIB_JPG_TURBO})
        set(BIC_INTERNAL_LIBJPEG_TURBO_SOURCES ${BIC_INTERNAL_LIBJPEG_TURBO_SOURCES}
            ${BIM_LIB_JPG_TURBO}/jaricom.c
            ${BIM_LIB_JPG_TURBO}/jcapimin.c
            ${BIM_LIB_JPG_TURBO}/jcapistd.c)

        internal_add_library(turbojpeg "${BIC_INTERNAL_LIBJPEG_TURBO_SOURCES}" "${BIC_INTERNAL_LIBJPEG_TURBO_HEADERS}" "${BIC_INTERNAL_LIBJPEG_TURBO_INSTALLHEADERS}" "${BIC_INTERNAL_LIBJPEG_TURBO_LINK_LIBRARIES}")

        set(LibJpegTurbo_LIBRARIES turbojpeg)
    elseif(LIBBIOIMAGE)
        find_package(LibJpegTurbo ${BIC_LIBJPEG_TURBO_REQUIRED_VERSION} REQUIRED)
        include_directories(${LibJpegTurbo_INCLUDE_DIRS})
    endif()
endif()


#---------------------------------------------------------------------
# libjpeg
#---------------------------------------------------------------------

if(NOT BIC_ENABLE_LIBJPEG_TURBO)
    if(BIC_INTERNAL_LIBJPEG)
        include_directories(${BIM_LIB_JPG})
        set(BIC_INTERNAL_LIBJPEG_SOURCES ${BIC_INTERNAL_LIBJPEG_SOURCES}
            ${BIM_LIB_JPG}/jaricom.c
            ${BIM_LIB_JPG}/jcapimin.c
            ${BIM_LIB_JPG}/jcapistd.c
            ${BIM_LIB_JPG}/jcarith.c
            ${BIM_LIB_JPG}/jccoefct.c
            ${BIM_LIB_JPG}/jccolor.c
            ${BIM_LIB_JPG}/jcdctmgr.c
            ${BIM_LIB_JPG}/jchuff.c
            ${BIM_LIB_JPG}/jcinit.c
            ${BIM_LIB_JPG}/jcmainct.c
            ${BIM_LIB_JPG}/jcmarker.c
            ${BIM_LIB_JPG}/jcmaster.c
            ${BIM_LIB_JPG}/jcomapi.c
            ${BIM_LIB_JPG}/jcparam.c
            ${BIM_LIB_JPG}/jcprepct.c
            ${BIM_LIB_JPG}/jcsample.c
            ${BIM_LIB_JPG}/jctrans.c
            ${BIM_LIB_JPG}/jdapimin.c
            ${BIM_LIB_JPG}/jdapistd.c
            ${BIM_LIB_JPG}/jdarith.c
            ${BIM_LIB_JPG}/jdatadst.c
            ${BIM_LIB_JPG}/jdatasrc.c
            ${BIM_LIB_JPG}/jdcoefct.c
            ${BIM_LIB_JPG}/jdcolor.c
            ${BIM_LIB_JPG}/jddctmgr.c
            ${BIM_LIB_JPG}/jdhuff.c
            ${BIM_LIB_JPG}/jdinput.c
            ${BIM_LIB_JPG}/jdmainct.c
            ${BIM_LIB_JPG}/jdmarker.c
            ${BIM_LIB_JPG}/jdmaster.c
            ${BIM_LIB_JPG}/jdmerge.c
            ${BIM_LIB_JPG}/jdpostct.c
            ${BIM_LIB_JPG}/jdsample.c
            ${BIM_LIB_JPG}/jdtrans.c
            ${BIM_LIB_JPG}/jerror.c
            ${BIM_LIB_JPG}/jfdctflt.c
            ${BIM_LIB_JPG}/jfdctfst.c
            ${BIM_LIB_JPG}/jfdctint.c
            ${BIM_LIB_JPG}/jidctflt.c
            ${BIM_LIB_JPG}/jidctfst.c
            ${BIM_LIB_JPG}/jidctint.c
            ${BIM_LIB_JPG}/jmemmgr.c
            ${BIM_LIB_JPG}/jquant1.c
            ${BIM_LIB_JPG}/jquant2.c
            ${BIM_LIB_JPG}/jutils.c
            ${BIM_LIB_JPG}/jmemansi.c)

        internal_add_library(jpeg "${BIC_INTERNAL_LIBJPEG_SOURCES}" "${BIC_INTERNAL_LIBJPEG_HEADERS}" "${BIC_INTERNAL_LIBJPEG_INSTALLHEADERS}" "${BIC_INTERNAL_LIBJPEG_LINK_LIBRARIES}")

        set(JPEG_LIBRARIES jpeg)
    elseif(LIBBIOIMAGE)
        find_package(JPEG REQUIRED)
        include_directories(${JPEG_INCLUDE_DIR})
    endif()
endif()


#---------------------------------------------------------------------
# libraw
#---------------------------------------------------------------------

if(BIC_INTERNAL_LIBRAW STREQUAL "AUTO")
    find_package(LibRaw ${BIC_LIBRAW_REQUIRED_VERSION})
    if(LibRaw_FOUND)
        set(BIC_INTERNAL_LIBRAW OFF)
        message("System libraw found in suitable version, disabling internal libraw.")
    else()
        set(BIC_INTERNAL_LIBRAW ON)
        message("System libraw not found (or found unsuitable version), enabling internal libraw.")
    endif()
endif()
if(BIC_INTERNAL_LIBRAW)
    add_definitions(-DLIBRAW_BUILDLIB -DLIBRAW_NODLL -DUSE_JPEG -DUSE_ZLIB -DUSE_LCMS2)
    include_directories(${BIM_LIB_RAW})

    set(BIC_INTERNAL_LIBRAW_SOURCES ${BIC_INTERNAL_LIBRAW_SOURCES}
        ${BIM_LIB_RAW}/src/libraw_c_api.cpp
        ${BIM_LIB_RAW}/src/libraw_cxx.cpp
        ${BIM_LIB_RAW}/src/libraw_datastream.cpp
        ${BIM_LIB_RAW}/internal/demosaic_packs.cpp
        ${BIM_LIB_RAW}/internal/dcraw_fileio.cpp
        ${BIM_LIB_RAW}/internal/dcraw_common.cpp)

    set(BIC_INTERNAL_LIBRAW_HEADERS ${BIC_INTERNAL_LIBRAW_HEADERS}
        ${BIM_LIB_RAW}/internal/libraw_internal_funcs.h
        ${BIM_LIB_RAW}/internal/var_defines.h
        ${BIM_LIB_RAW}/internal/defines.h)

    set(BIC_INTERNAL_LIBRAW_LINK_LIBRARIES ${BIC_INTERNAL_LIBRAW_LINK_LIBRARIES} jpeg lcms2 z)
    if(WIN32)
        set(BIC_INTERNAL_LIBRAW_LINK_LIBRARIES ${BIC_INTERNAL_LIBRAW_LINK_LIBRARIES} ws2_32)
    endif()
    internal_add_library(raw "${BIC_INTERNAL_LIBRAW_SOURCES}" "${BIC_INTERNAL_LIBRAW_HEADERS}" "${BIC_INTERNAL_LIBRAW_INSTALLHEADERS}" "${BIC_INTERNAL_LIBRAW_LINK_LIBRARIES}")

    # TODO FIXME: merge this linker command with the automatic system library detection below:
    set(LibRaw_LIBRARIES raw)
elseif(LIBBIOIMAGE)
    find_package(LibRaw ${BIC_LIBRAW_REQUIRED_VERSION} REQUIRED)
    include_directories(${LibRaw_INCLUDE_DIR})
endif()


#---------------------------------------------------------------------
# libTiff
#---------------------------------------------------------------------

if(BIC_INTERNAL_LIBTIFF STREQUAL "AUTO")
    find_package(TIFF ${BIC_LIBTIFF_REQUIRED_VERSION})
    if(TIFF_FOUND)
        set(BIC_INTERNAL_LIBTIFF OFF)
        message("System libtiff found in suitable version, disabling internal libtiff.")
    else()
        set(BIC_INTERNAL_LIBTIFF ON)
        message("System libtiff not found (or found unsuitable version), enabling internal libtiff.")
    endif()
endif()
if(BIC_INTERNAL_LIBTIFF)
    include_directories(${BIM_LIB_TIF})
    include_directories(${BIM_LIB_TIF}/project/qt)

    set(BIC_INTERNAL_LIBTIFF_SOURCES ${BIC_INTERNAL_LIBTIFF_SOURCES}
        ${BIM_LIB_TIF}/tif_fax3sm.c
        ${BIM_LIB_TIF}/tif_aux.c
        ${BIM_LIB_TIF}/tif_close.c
        ${BIM_LIB_TIF}/tif_codec.c
        ${BIM_LIB_TIF}/tif_color.c
        ${BIM_LIB_TIF}/tif_compress.c
        ${BIM_LIB_TIF}/tif_dir.c
        ${BIM_LIB_TIF}/tif_dirinfo.c
        ${BIM_LIB_TIF}/tif_dirread.c
        ${BIM_LIB_TIF}/tif_dirwrite.c
        ${BIM_LIB_TIF}/tif_dumpmode.c
        ${BIM_LIB_TIF}/tif_error.c
        ${BIM_LIB_TIF}/tif_extension.c
        ${BIM_LIB_TIF}/tif_fax3.c
        ${BIM_LIB_TIF}/tif_flush.c
        ${BIM_LIB_TIF}/tif_getimage.c
        ${BIM_LIB_TIF}/tif_jpeg.c
        ${BIM_LIB_TIF}/tif_luv.c
        ${BIM_LIB_TIF}/tif_lzw.c
        ${BIM_LIB_TIF}/tif_next.c
        ${BIM_LIB_TIF}/tif_open.c
        ${BIM_LIB_TIF}/tif_packbits.c
        ${BIM_LIB_TIF}/tif_pixarlog.c
        ${BIM_LIB_TIF}/tif_predict.c
        ${BIM_LIB_TIF}/tif_print.c
        ${BIM_LIB_TIF}/tif_read.c
        ${BIM_LIB_TIF}/tif_strip.c
        ${BIM_LIB_TIF}/tif_swab.c
        ${BIM_LIB_TIF}/tif_thunder.c
        ${BIM_LIB_TIF}/tif_tile.c
        ${BIM_LIB_TIF}/tif_version.c
        ${BIM_LIB_TIF}/tif_warning.c
        ${BIM_LIB_TIF}/tif_write.c
        ${BIM_LIB_TIF}/tif_zip.c
        ${BIM_LIB_TIF}/tif_stream.cxx
        ${BIM_LIB_TIF}/tif_lzma.c)

    if(BIC_INTERNAL_LZMA)
        set(BIC_INTERNAL_LIBTIFF_SOURCES ${BIC_INTERNAL_LIBTIFF_SOURCES}
            ${BIM_LIB_TIF}/tif_fax3sm.c
            ${BIM_LIB_TIF}/tif_aux.c
            ${BIM_LIB_TIF}/tif_close.c
            ${BIM_LIB_TIF}/tif_codec.c)
    endif()
    if(UNIX)
        set(BIC_INTERNAL_LIBTIFF_SOURCES ${BIC_INTERNAL_LIBTIFF_SOURCES}
            ${BIM_LIB_TIF}/tif_unix.c)
    elseif(WIN32)
        set(BIC_INTERNAL_LIBTIFF_SOURCES ${BIC_INTERNAL_LIBTIFF_SOURCES}
            ${BIM_LIB_TIF}/tif_win32.c)
    endif()

    set(BIC_INTERNAL_LIBTIFF_INSTALLHEADERS ${BIC_INTERNAL_LIBTIFF_INSTALLHEADERS}
        ${BIM_LIB_TIF}/tiff.h
        ${BIM_LIB_TIF}/tiffconf.h
        ${BIM_LIB_TIF}/tiffio.h
        ${BIM_LIB_TIF}/tiffio.hxx
        ${BIM_LIB_TIF}/tiffvers.h)

    set(BIC_INTERNAL_LIBTIFF_HEADERS ${BIC_INTERNAL_LIBTIFF_HEADERS}
        ${BIM_LIB_TIF}/t4.h
        ${BIM_LIB_TIF}/tif_config.h
        ${BIM_LIB_TIF}/tif_config.vc.h
        ${BIM_LIB_TIF}/tif_config.wince.h
        ${BIM_LIB_TIF}/tif_dir.h
        ${BIM_LIB_TIF}/tif_fax3.h
        ${BIM_LIB_TIF}/tiffconf.vc.h
        ${BIM_LIB_TIF}/tiffconf.wince.h
        ${BIM_LIB_TIF}/tiffiop.h
        ${BIM_LIB_TIF}/tif_predict.h
        ${BIM_LIB_TIF}/uvcode.h)

    #set(BIC_INTERNAL_LIBTIFF_LINK_LIBRARIES ${BIC_INTERNAL_LIBTIFF_LINK_LIBRARIES} jpeg lzma z)
    internal_add_library(tiff "${BIC_INTERNAL_LIBTIFF_SOURCES}" "${BIC_INTERNAL_LIBTIFF_HEADERS}" "${BIC_INTERNAL_LIBTIFF_INSTALLHEADERS}" "${BIC_INTERNAL_LIBTIFF_LINK_LIBRARIES}")

    set(TIFF_LIBRARIES tiff)
elseif(LIBBIOIMAGE OR BIC_INTERNAL_LIBGEOTIFF)
    find_package(TIFF ${BIC_LIBTIFF_REQUIRED_VERSION} REQUIRED)
    include_directories(${TIFF_INCLUDE_DIR})
endif()


#---------------------------------------------------------------------
# Proj.4
#---------------------------------------------------------------------

if(BIC_INTERNAL_PROJ4 STREQUAL "AUTO")
    find_package(PROJ4 ${BIC_PROJ4_REQUIRED_VERSION})
    if(PROJ4_FOUND)
        set(BIC_INTERNAL_PROJ4 OFF)
        message("System proj4 found in suitable version, disabling internal proj4.")
    else()
        set(BIC_INTERNAL_PROJ4 ON)
        message("System proj4 not found (or found unsuitable version), enabling internal proj4.")
    endif()
endif()
if(BIC_INTERNAL_PROJ4)
    include_directories(${BIM_LIB_PROJ4})

    set(BIC_INTERNAL_PROJ4_SOURCES ${BIC_INTERNAL_PROJ4_SOURCES}
        ${BIM_LIB_PROJ4}/aasincos.c
        ${BIM_LIB_PROJ4}/adjlon.c
        ${BIM_LIB_PROJ4}/bch2bps.c
        ${BIM_LIB_PROJ4}/bchgen.c
        ${BIM_LIB_PROJ4}/biveval.c
        ${BIM_LIB_PROJ4}/dmstor.c
        ${BIM_LIB_PROJ4}/emess.c
        ${BIM_LIB_PROJ4}/gen_cheb.c
        ${BIM_LIB_PROJ4}/geocent.c
        ${BIM_LIB_PROJ4}/geod_for.c
        ${BIM_LIB_PROJ4}/geod_inv.c
        ${BIM_LIB_PROJ4}/geod_set.c
        ${BIM_LIB_PROJ4}/jniproj.c
        ${BIM_LIB_PROJ4}/mk_cheby.c
        ${BIM_LIB_PROJ4}/nad_cvt.c
        ${BIM_LIB_PROJ4}/nad_init.c
        ${BIM_LIB_PROJ4}/nad_intr.c
        ${BIM_LIB_PROJ4}/PJ_aea.c
        ${BIM_LIB_PROJ4}/PJ_aeqd.c
        ${BIM_LIB_PROJ4}/PJ_airy.c
        ${BIM_LIB_PROJ4}/PJ_aitoff.c
        ${BIM_LIB_PROJ4}/pj_apply_gridshift.c
        ${BIM_LIB_PROJ4}/pj_apply_vgridshift.c
        ${BIM_LIB_PROJ4}/PJ_august.c
        ${BIM_LIB_PROJ4}/pj_auth.c
        ${BIM_LIB_PROJ4}/PJ_bacon.c
        ${BIM_LIB_PROJ4}/PJ_bipc.c
        ${BIM_LIB_PROJ4}/PJ_boggs.c
        ${BIM_LIB_PROJ4}/PJ_bonne.c
        ${BIM_LIB_PROJ4}/PJ_cass.c
        ${BIM_LIB_PROJ4}/PJ_cc.c
        ${BIM_LIB_PROJ4}/PJ_cea.c
        ${BIM_LIB_PROJ4}/PJ_chamb.c
        ${BIM_LIB_PROJ4}/PJ_collg.c
        ${BIM_LIB_PROJ4}/PJ_crast.c
        ${BIM_LIB_PROJ4}/pj_ctx.c
        ${BIM_LIB_PROJ4}/pj_datums.c
        ${BIM_LIB_PROJ4}/pj_datum_set.c
        ${BIM_LIB_PROJ4}/PJ_denoy.c
        ${BIM_LIB_PROJ4}/pj_deriv.c
        ${BIM_LIB_PROJ4}/PJ_eck1.c
        ${BIM_LIB_PROJ4}/PJ_eck2.c
        ${BIM_LIB_PROJ4}/PJ_eck3.c
        ${BIM_LIB_PROJ4}/PJ_eck4.c
        ${BIM_LIB_PROJ4}/PJ_eck5.c
        ${BIM_LIB_PROJ4}/pj_ellps.c
        ${BIM_LIB_PROJ4}/pj_ell_set.c
        ${BIM_LIB_PROJ4}/PJ_eqc.c
        ${BIM_LIB_PROJ4}/PJ_eqdc.c
        ${BIM_LIB_PROJ4}/pj_errno.c
        ${BIM_LIB_PROJ4}/pj_factors.c
        ${BIM_LIB_PROJ4}/PJ_fahey.c
        ${BIM_LIB_PROJ4}/PJ_fouc_s.c
        ${BIM_LIB_PROJ4}/pj_fwd.c
        ${BIM_LIB_PROJ4}/PJ_gall.c
        ${BIM_LIB_PROJ4}/pj_gauss.c
        ${BIM_LIB_PROJ4}/pj_geocent.c
        ${BIM_LIB_PROJ4}/PJ_geos.c
        ${BIM_LIB_PROJ4}/PJ_gins8.c
        ${BIM_LIB_PROJ4}/PJ_gnom.c
        ${BIM_LIB_PROJ4}/PJ_gn_sinu.c
        ${BIM_LIB_PROJ4}/PJ_goode.c
        ${BIM_LIB_PROJ4}/pj_gridinfo.c
        ${BIM_LIB_PROJ4}/pj_gridlist.c
        ${BIM_LIB_PROJ4}/PJ_gstmerc.c
        ${BIM_LIB_PROJ4}/PJ_hammer.c
        ${BIM_LIB_PROJ4}/PJ_hatano.c
        ${BIM_LIB_PROJ4}/PJ_healpix.c
        ${BIM_LIB_PROJ4}/PJ_igh.c
        ${BIM_LIB_PROJ4}/PJ_imw_p.c
        ${BIM_LIB_PROJ4}/pj_init.c
        ${BIM_LIB_PROJ4}/pj_initcache.c
        ${BIM_LIB_PROJ4}/pj_inv.c
        ${BIM_LIB_PROJ4}/PJ_isea.c
        ${BIM_LIB_PROJ4}/PJ_krovak.c
        ${BIM_LIB_PROJ4}/PJ_labrd.c
        ${BIM_LIB_PROJ4}/PJ_laea.c
        ${BIM_LIB_PROJ4}/PJ_lagrng.c
        ${BIM_LIB_PROJ4}/PJ_larr.c
        ${BIM_LIB_PROJ4}/PJ_lask.c
        ${BIM_LIB_PROJ4}/pj_latlong.c
        ${BIM_LIB_PROJ4}/PJ_lcc.c
        ${BIM_LIB_PROJ4}/PJ_lcca.c
        ${BIM_LIB_PROJ4}/pj_list.c
        ${BIM_LIB_PROJ4}/pj_log.c
        ${BIM_LIB_PROJ4}/PJ_loxim.c
        ${BIM_LIB_PROJ4}/PJ_lsat.c
        ${BIM_LIB_PROJ4}/pj_malloc.c
        ${BIM_LIB_PROJ4}/PJ_mbtfpp.c
        ${BIM_LIB_PROJ4}/PJ_mbtfpq.c
        ${BIM_LIB_PROJ4}/PJ_mbt_fps.c
        ${BIM_LIB_PROJ4}/PJ_merc.c
        ${BIM_LIB_PROJ4}/PJ_mill.c
        ${BIM_LIB_PROJ4}/pj_mlfn.c
        ${BIM_LIB_PROJ4}/PJ_mod_ster.c
        ${BIM_LIB_PROJ4}/PJ_moll.c
        ${BIM_LIB_PROJ4}/pj_msfn.c
        ${BIM_LIB_PROJ4}/pj_mutex.c
        ${BIM_LIB_PROJ4}/PJ_natearth.c
        ${BIM_LIB_PROJ4}/PJ_nell.c
        ${BIM_LIB_PROJ4}/PJ_nell_h.c
        ${BIM_LIB_PROJ4}/PJ_nocol.c
        ${BIM_LIB_PROJ4}/PJ_nsper.c
        ${BIM_LIB_PROJ4}/PJ_nzmg.c
        ${BIM_LIB_PROJ4}/PJ_ob_tran.c
        ${BIM_LIB_PROJ4}/PJ_ocea.c
        ${BIM_LIB_PROJ4}/PJ_oea.c
        ${BIM_LIB_PROJ4}/PJ_omerc.c
        ${BIM_LIB_PROJ4}/pj_open_lib.c
        ${BIM_LIB_PROJ4}/PJ_ortho.c
        ${BIM_LIB_PROJ4}/pj_param.c
        ${BIM_LIB_PROJ4}/pj_phi2.c
        ${BIM_LIB_PROJ4}/PJ_poly.c
        ${BIM_LIB_PROJ4}/pj_pr_list.c
        ${BIM_LIB_PROJ4}/PJ_putp2.c
        ${BIM_LIB_PROJ4}/PJ_putp3.c
        ${BIM_LIB_PROJ4}/PJ_putp4p.c
        ${BIM_LIB_PROJ4}/PJ_putp5.c
        ${BIM_LIB_PROJ4}/PJ_putp6.c
        ${BIM_LIB_PROJ4}/pj_qsfn.c
        ${BIM_LIB_PROJ4}/pj_release.c
        ${BIM_LIB_PROJ4}/PJ_robin.c
        ${BIM_LIB_PROJ4}/PJ_rpoly.c
        ${BIM_LIB_PROJ4}/PJ_sconics.c
        ${BIM_LIB_PROJ4}/PJ_somerc.c
        ${BIM_LIB_PROJ4}/PJ_stere.c
        ${BIM_LIB_PROJ4}/PJ_sterea.c
        ${BIM_LIB_PROJ4}/pj_strerrno.c
        ${BIM_LIB_PROJ4}/PJ_sts.c
        ${BIM_LIB_PROJ4}/PJ_tcc.c
        ${BIM_LIB_PROJ4}/PJ_tcea.c
        ${BIM_LIB_PROJ4}/PJ_tmerc.c
        ${BIM_LIB_PROJ4}/PJ_tpeqd.c
        ${BIM_LIB_PROJ4}/pj_transform.c
        ${BIM_LIB_PROJ4}/pj_tsfn.c
        ${BIM_LIB_PROJ4}/pj_units.c
        ${BIM_LIB_PROJ4}/PJ_urm5.c
        ${BIM_LIB_PROJ4}/PJ_urmfps.c
        ${BIM_LIB_PROJ4}/pj_utils.c
        ${BIM_LIB_PROJ4}/PJ_vandg.c
        ${BIM_LIB_PROJ4}/PJ_vandg2.c
        ${BIM_LIB_PROJ4}/PJ_vandg4.c
        ${BIM_LIB_PROJ4}/PJ_wag2.c
        ${BIM_LIB_PROJ4}/PJ_wag3.c
        ${BIM_LIB_PROJ4}/PJ_wag7.c
        ${BIM_LIB_PROJ4}/PJ_wink1.c
        ${BIM_LIB_PROJ4}/PJ_wink2.c
        ${BIM_LIB_PROJ4}/pj_zpoly1.c
        ${BIM_LIB_PROJ4}/proj.c
        ${BIM_LIB_PROJ4}/proj_etmerc.c
        ${BIM_LIB_PROJ4}/proj_mdist.c
        ${BIM_LIB_PROJ4}/proj_rouss.c
        ${BIM_LIB_PROJ4}/p_series.c
        ${BIM_LIB_PROJ4}/rtodms.c
        ${BIM_LIB_PROJ4}/vector1.c)

    set(BIC_INTERNAL_PROJ4_INSTALLHEADERS ${BIC_INTERNAL_PROJ4_INSTALLHEADERS}
        ${BIM_LIB_PROJ4}/projects.h
        ${BIM_LIB_PROJ4}/geodesic.h
        ${BIM_LIB_PROJ4}/org_proj4_Projections.h
        ${BIM_LIB_PROJ4}/proj_api.h)

    internal_add_library(proj "${BIC_INTERNAL_PROJ4_SOURCES}" "${BIC_INTERNAL_PROJ4_HEADERS}" "${BIC_INTERNAL_PROJ4_INSTALLHEADERS}" "${BIC_INTERNAL_PROJ4_LINK_LIBRARIES}")

    set(PROJ4_LIBRARIES proj)
elseif(LIBBIOIMAGE OR BIC_INTERNAL_LIBGEOTIFF)
    find_package(PROJ4 ${BIC_PROJ4_REQUIRED_VERSION} REQUIRED)
endif()


#---------------------------------------------------------------------
# libGeoTiff
#---------------------------------------------------------------------

if(BIC_INTERNAL_LIBGEOTIFF)
    include_directories(${BIM_LIB_GEOTIF})
    include_directories(${BIM_FMTS}/tiff)
    set(BIC_INTERNAL_LIBGEOTIFF_SOURCES ${BIC_INTERNAL_LIBGEOTIFF_SOURCES}
        ${BIM_LIB_GEOTIF}/cpl_csv.c
        ${BIM_LIB_GEOTIF}/cpl_serv.c
        ${BIM_LIB_GEOTIF}/geotiff_proj4.c
        ${BIM_LIB_GEOTIF}/geo_extra.c
        ${BIM_LIB_GEOTIF}/geo_free.c
        ${BIM_LIB_GEOTIF}/geo_get.c
        ${BIM_LIB_GEOTIF}/geo_names.c
        ${BIM_LIB_GEOTIF}/geo_new.c
        ${BIM_LIB_GEOTIF}/geo_normalize.c
        ${BIM_LIB_GEOTIF}/geo_print.c
        ${BIM_LIB_GEOTIF}/geo_set.c
        ${BIM_LIB_GEOTIF}/geo_simpletags.c
        ${BIM_LIB_GEOTIF}/geo_strtod.c
        ${BIM_LIB_GEOTIF}/geo_tiffp.c
        ${BIM_LIB_GEOTIF}/geo_trans.c
        ${BIM_LIB_GEOTIF}/geo_write.c)

    set(BIC_INTERNAL_LIBGEOTIFF_INSTALLHEADERS ${BIC_INTERNAL_LIBGEOTIFF_INSTALLHEADERS}
        ${BIM_LIB_GEOTIF}/cpl_serv.h
        ${BIM_LIB_GEOTIF}/epsg_datum.inc
        ${BIM_LIB_GEOTIF}/epsg_ellipse.inc
        ${BIM_LIB_GEOTIF}/epsg_gcs.inc
        ${BIM_LIB_GEOTIF}/epsg_pcs.inc
        ${BIM_LIB_GEOTIF}/epsg_pm.inc
        ${BIM_LIB_GEOTIF}/epsg_proj.inc
        ${BIM_LIB_GEOTIF}/epsg_units.inc
        ${BIM_LIB_GEOTIF}/epsg_vertcs.inc
        ${BIM_LIB_GEOTIF}/geo_config.h
        ${BIM_LIB_GEOTIF}/geo_ctrans.inc
        ${BIM_LIB_GEOTIF}/geo_incode_defs.h
        ${BIM_LIB_GEOTIF}/geo_keyp.h
        ${BIM_LIB_GEOTIF}/geo_normalize.h
        ${BIM_LIB_GEOTIF}/geo_simpletags.h
        ${BIM_LIB_GEOTIF}/geo_tiffp.h
        ${BIM_LIB_GEOTIF}/geokeys.h
        ${BIM_LIB_GEOTIF}/geokeys.inc
        ${BIM_LIB_GEOTIF}/geonames.h
        ${BIM_LIB_GEOTIF}/geotiff.h
        ${BIM_LIB_GEOTIF}/geotiffio.h
        ${BIM_LIB_GEOTIF}/geovalues.h)

    # geotiff links proj4 and libtiff:
    set(BIC_INTERNAL_LIBGEOTIFF_LINK_LIBRARIES ${BIC_INTERNAL_LIBGEOTIFF_LINK_LIBRARIES} ${PROJ4_LIBRARIES} ${TIFF_LIBRARIES})

    internal_add_library(geotiff "${BIC_INTERNAL_LIBGEOTIFF_SOURCES}" "${BIC_INTERNAL_LIBGEOTIFF_HEADERS}" "${BIC_INTERNAL_LIBGEOTIFF_INSTALLHEADERS}" "${BIC_INTERNAL_LIBGEOTIFF_LINK_LIBRARIES}")

    set(GEOTIFF_LIBRARIES geotiff)
elseif(LIBBIOIMAGE)
    find_package(GeoTIFF REQUIRED)
    include_directories(${GEOTIFF_INCLUDE_DIR})
endif()


#---------------------------------------------------------------------
# libPng
#---------------------------------------------------------------------

if(BIC_INTERNAL_LIBPNG)
    include_directories(${BIM_LIB_PNG})

    # by default disable intel asm code
    if(UNIX)
        add_definitions(-DPNG_NO_ASSEMBLER_CODE -DPNG_USE_PNGVCRD)
    endif()

    set(BIC_INTERNAL_PNG_SOURCES ${BIC_INTERNAL_PNG_SOURCES}
        ${BIM_LIB_PNG}/png.c
        ${BIM_LIB_PNG}/pngerror.c
        ${BIM_LIB_PNG}/pngget.c
        ${BIM_LIB_PNG}/pngmem.c
        ${BIM_LIB_PNG}/pngpread.c
        ${BIM_LIB_PNG}/pngread.c
        ${BIM_LIB_PNG}/pngrio.c
        ${BIM_LIB_PNG}/pngrtran.c
        ${BIM_LIB_PNG}/pngrutil.c
        ${BIM_LIB_PNG}/pngset.c
        ${BIM_LIB_PNG}/pngtrans.c
        ${BIM_LIB_PNG}/pngwio.c
        ${BIM_LIB_PNG}/pngwrite.c
        ${BIM_LIB_PNG}/pngwtran.c
        ${BIM_LIB_PNG}/pngwutil.c)

    set(BIC_INTERNAL_PNG_INSTALLHEADERS ${BIC_INTERNAL_PNG_INSTALLHEADERS}
        ${BIM_LIB_PNG}/png.h
        ${BIM_LIB_PNG}/pngconf.h
        ${BIM_LIB_PNG}/pnglibconf.h)

    internal_add_library(png "${BIC_INTERNAL_PNG_SOURCES}" "${BIC_INTERNAL_PNG_HEADERS}" "${BIC_INTERNAL_PNG_INSTALLHEADERS}" "${BIC_INTERNAL_PNG_LINK_LIBRARIES}")

    set(PNG_LIBRARY png)
elseif(LIBBIOIMAGE)
    find_package(PNG REQUIRED)
    if(NOT "${PNG_VERSION_STRING}" MATCHES "^1.[4-9]|^[2.9]")
        message(FATAL_ERROR "Found png ${PNG_VERSION_STRING} is not sufficient, requiring at least png 1.4")
    endif()
    include_directories(${PNG_INCLUDE_DIR})
endif()


#---------------------------------------------------------------------
# exiv2
#---------------------------------------------------------------------

if(BIC_INTERNAL_EXIV2 STREQUAL "AUTO")
    find_package(Exiv2 ${BIC_EXIV2_REQUIRED_VERSION})
    if(EXIV2_FOUND)
        set(BIC_INTERNAL_EXIV2 OFF)
        message("System Exiv2 found in suitable version, disabling internal Exiv2.")
    else()
        set(BIC_INTERNAL_EXIV2 ON)
        message("System Exiv2 not found (or found unsuitable version), enabling internal Exiv2.")
    endif()
endif()
if(BIC_INTERNAL_EXIV2)
    include_directories(${BIM_LIB_EXIV2})
    include_directories(${BIM_LIB_EXIV2}/exiv2)

    set(BIC_INTERNAL_EXIV2_SOURCES ${BIC_INTERNAL_EXIV2_SOURCES}
        ${BIM_LIB_EXIV2}/exiv2/asfvideo.cpp
        ${BIM_LIB_EXIV2}/exiv2/basicio.cpp
        ${BIM_LIB_EXIV2}/exiv2/bmpimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/canonmn.cpp
        ${BIM_LIB_EXIV2}/exiv2/casiomn.cpp
        ${BIM_LIB_EXIV2}/exiv2/convert.cpp
        ${BIM_LIB_EXIV2}/exiv2/cr2image.cpp
        ${BIM_LIB_EXIV2}/exiv2/crwimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/datasets.cpp
        ${BIM_LIB_EXIV2}/exiv2/easyaccess.cpp
        ${BIM_LIB_EXIV2}/exiv2/epsimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/error.cpp
        ${BIM_LIB_EXIV2}/exiv2/exif.cpp
        ${BIM_LIB_EXIV2}/exiv2/fujimn.cpp
        ${BIM_LIB_EXIV2}/exiv2/futils.cpp
        ${BIM_LIB_EXIV2}/exiv2/gifimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/http.cpp
        ${BIM_LIB_EXIV2}/exiv2/image.cpp
        ${BIM_LIB_EXIV2}/exiv2/iptc.cpp
        ${BIM_LIB_EXIV2}/exiv2/jp2image.cpp
        ${BIM_LIB_EXIV2}/exiv2/jpgimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/makernote.cpp
        ${BIM_LIB_EXIV2}/exiv2/matroskavideo.cpp
        ${BIM_LIB_EXIV2}/exiv2/metadatum.cpp
        ${BIM_LIB_EXIV2}/exiv2/minoltamn.cpp
        ${BIM_LIB_EXIV2}/exiv2/mrwimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/nikonmn.cpp
        ${BIM_LIB_EXIV2}/exiv2/olympusmn.cpp
        ${BIM_LIB_EXIV2}/exiv2/orfimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/panasonicmn.cpp
        ${BIM_LIB_EXIV2}/exiv2/pentaxmn.cpp
        ${BIM_LIB_EXIV2}/exiv2/pgfimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/pngchunk.cpp
        ${BIM_LIB_EXIV2}/exiv2/pngimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/preview.cpp
        ${BIM_LIB_EXIV2}/exiv2/properties.cpp
        ${BIM_LIB_EXIV2}/exiv2/quicktimevideo.cpp
        ${BIM_LIB_EXIV2}/exiv2/psdimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/rafimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/riffvideo.cpp
        ${BIM_LIB_EXIV2}/exiv2/rw2image.cpp
        ${BIM_LIB_EXIV2}/exiv2/sigmamn.cpp
        ${BIM_LIB_EXIV2}/exiv2/samsungmn.cpp
        ${BIM_LIB_EXIV2}/exiv2/sonymn.cpp
        ${BIM_LIB_EXIV2}/exiv2/ssh.cpp
        ${BIM_LIB_EXIV2}/exiv2/tags.cpp
        ${BIM_LIB_EXIV2}/exiv2/tgaimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/tiffcomposite.cpp
        ${BIM_LIB_EXIV2}/exiv2/tiffimage.cpp
        ${BIM_LIB_EXIV2}/exiv2/tiffvisitor.cpp
        ${BIM_LIB_EXIV2}/exiv2/types.cpp
        ${BIM_LIB_EXIV2}/exiv2/utils.cpp
        ${BIM_LIB_EXIV2}/exiv2/value.cpp
        ${BIM_LIB_EXIV2}/exiv2/version.cpp
        ${BIM_LIB_EXIV2}/exiv2/xmp.cpp
        ${BIM_LIB_EXIV2}/exiv2/xmpsidecar.cpp)

    set(BIC_INTERNAL_EXIV2_INSTALLHEADERS ${BIC_INTERNAL_EXIV2_INSTALLHEADERS}
        ${BIM_LIB_EXIV2}/exiv2/basicio.hpp
        ${BIM_LIB_EXIV2}/exiv2/bmpimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/config.h
        ${BIM_LIB_EXIV2}/exiv2/convert.hpp
        ${BIM_LIB_EXIV2}/exiv2/cr2image.hpp
        ${BIM_LIB_EXIV2}/exiv2/crwimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/datasets.hpp
        ${BIM_LIB_EXIV2}/exiv2/easyaccess.hpp
        ${BIM_LIB_EXIV2}/exiv2/epsimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/error.hpp
        ${BIM_LIB_EXIV2}/exiv2/exif.hpp
        ${BIM_LIB_EXIV2}/exiv2/exiv2.hpp
        ${BIM_LIB_EXIV2}/exiv2/futils.hpp
        ${BIM_LIB_EXIV2}/exiv2/gifimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/http.hpp
        ${BIM_LIB_EXIV2}/exiv2/image.hpp
        ${BIM_LIB_EXIV2}/exiv2/iptc.hpp
        ${BIM_LIB_EXIV2}/exiv2/jp2image.hpp
        ${BIM_LIB_EXIV2}/exiv2/jpgimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/metadatum.hpp
        ${BIM_LIB_EXIV2}/exiv2/mrwimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/orfimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/pgfimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/pngimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/preview.hpp
        ${BIM_LIB_EXIV2}/exiv2/properties.hpp
        ${BIM_LIB_EXIV2}/exiv2/psdimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/rafimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/rw2image.hpp
        ${BIM_LIB_EXIV2}/exiv2/svn_version.h
        ${BIM_LIB_EXIV2}/exiv2/tags.hpp
        ${BIM_LIB_EXIV2}/exiv2/tgaimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/tiffimage.hpp
        ${BIM_LIB_EXIV2}/exiv2/types.hpp
        ${BIM_LIB_EXIV2}/exiv2/value.hpp
        ${BIM_LIB_EXIV2}/exiv2/version.hpp
        ${BIM_LIB_EXIV2}/exiv2/xmp.hpp
        ${BIM_LIB_EXIV2}/exiv2/xmpsidecar.hpp)

    set(BIC_INTERNAL_EXIV2_HEADERS ${BIC_INTERNAL_EXIV2_HEADERS}
        ${BIM_LIB_EXIV2}/exiv2/actions.hpp
        ${BIM_LIB_EXIV2}/exiv2/asfvideo.hpp
        ${BIM_LIB_EXIV2}/exiv2/canonmn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/casiomn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/cr2image_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/crwimage_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/exiv2app.hpp
        ${BIM_LIB_EXIV2}/exiv2/fujimn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/getopt_win32.h
        ${BIM_LIB_EXIV2}/exiv2/i18n.h
        ${BIM_LIB_EXIV2}/exiv2/image_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/makernote_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/matroskavideo.hpp
        ${BIM_LIB_EXIV2}/exiv2/minoltamn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/nikonmn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/olympusmn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/orfimage_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/panasonicmn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/pentaxmn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/pngchunk_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/private.h
        ${BIM_LIB_EXIV2}/exiv2/quicktimevideo.hpp
        ${BIM_LIB_EXIV2}/exiv2/rcsid_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/riffvideo.hpp
        ${BIM_LIB_EXIV2}/exiv2/rw2image_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/samsungmn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/sigmamn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/sonymn_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/ssh.hpp
        ${BIM_LIB_EXIV2}/exiv2/tags_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/tiffcomposite_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/tifffwd_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/tiffimage_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/tiffvisitor_int.hpp
        ${BIM_LIB_EXIV2}/exiv2/timegm.h
        ${BIM_LIB_EXIV2}/exiv2/tzfile.h
        ${BIM_LIB_EXIV2}/exiv2/utils.hpp
        ${BIM_LIB_EXIV2}/exiv2/utilsvideo.hpp)

    internal_add_library(exiv2 "${BIC_INTERNAL_EXIV2_SOURCES}" "${BIC_INTERNAL_EXIV2_HEADERS}" "${BIC_INTERNAL_EXIV2_INSTALLHEADERS}" "${BIC_INTERNAL_EXIV2_LINK_LIBRARIES}")
    target_compile_definitions(exiv2 PUBLIC SUPPRESS_WARNINGS _FILE_OFFSET_BITS=64 EXV_BUILDING_LIB=1 DLL_EXPORT)

    set(EXIV2_LIBRARIES exiv2)
elseif(LIBBIOIMAGE)
    find_package(Exiv2 ${BIC_EXIV2_REQUIRED_VERSION} REQUIRED)
    include_directories(${EXIV2_INCLUDE_DIR})
endif()
if(BIC_INTERNAL_EXIV2 OR LIBBIOIMAGE)
    # Find the Iconv library that is a required dependency of exiv2
    if(LIBBIOIMAGE AND BIC_ENABLE_QT)
        find_package(Iconv REQUIRED)
        include_directories(${ICONV_INCLUDE_DIR})
    endif()
endif()



#---------------------------------------------------------------------
# jxrlib
#---------------------------------------------------------------------

if(BIC_ENABLE_JXRLIB)
    if(BIC_INTERNAL_JXRLIB STREQUAL "AUTO")
        find_package(JXR ${BIC_JXRLIB_REQUIRED_VERSION})
        if(JXR_FOUND)
            set(BIC_INTERNAL_JXRLIB OFF)
            message("System jxrlib found in suitable version, disabling internal jxrlib.")
        else()
            set(BIC_INTERNAL_JXRLIB ON)
            message("System jxrlib not found (or found unsuitable version), enabling internal jxrlib.")
        endif()
    endif()
    add_definitions(-D__ANSI__ -DDISABLE_PERF_MEASUREMENT)
    if(BIC_INTERNAL_JXRLIB)
        if(NOT MINGW)
            include_directories(${BIM_LIB_JXRLIB}/common/include)
            include_directories(${BIM_LIB_JXRLIB}/image/sys)
        endif()
        include_directories(${BIM_LIB_JXRLIB}/jxrgluelib)

        if(NOT EXISTS ${BIM_LIBS_PLTFM}/libjxrglue.a)
            message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
        endif()
        set(JXR_LIBRARIES
            ${BIM_LIBS_PLTFM}/libjxrglue.a
            ${BIM_LIBS_PLTFM}/libjpegxr.a)

        #set(BIC_INTERNAL_JXRLIB_SOURCES ${BIC_INTERNAL_JXRLIB_SOURCES}
        #    ${BIM_LIB_JXRLIB}/)

        #set(BIC_INTERNAL_JXRLIB_INSTALLHEADERS ${BIC_INTERNAL_JXRLIB_INSTALLHEADERS}
        #    ${BIM_LIB_JXRLIB}/)

        #set(BIC_INTERNAL_JXRLIB_HEADERS ${BIC_INTERNAL_JXRLIB_HEADERS}
        #    ${BIM_LIB_JXRLIB}/)

        #internal_add_library(jxrglue "${BIC_INTERNAL_JXRLIB_SOURCES}" "${BIC_INTERNAL_JXRLIB_HEADERS}" "${BIC_INTERNAL_JXRLIB_INSTALLHEADERS}" "${BIC_INTERNAL_JXRLIB_LINK_LIBRARIES}")
        #target_compile_definitions(jxrglue PUBLIC SUPPRESS_WARNINGS _FILE_OFFSET_BITS=64 EXV_BUILDING_LIB=1 DLL_EXPORT)
    elseif(LIBBIOIMAGE)
        find_package(JXR ${BIC_JXRLIB_REQUIRED_VERSION} REQUIRED)
        include_directories(${JXR_INCLUDE_DIR})
    endif()
endif()


#---------------------------------------------------------------------
# libwebp
#---------------------------------------------------------------------

if(BIC_INTERNAL_LIBWEBP STREQUAL "AUTO")
    find_package(WebP ${BIC_LIBWEBP_REQUIRED_VERSION})
    if(WebP_WebP_FOUND AND WebP_DeMux_FOUND AND WebP_Mux_FOUND)
        set(BIC_INTERNAL_LIBWEBP OFF)
        message("System WebP found in suitable version, disabling internal WebP.")
    else()
        set(BIC_INTERNAL_LIBWEBP ON)
        message("System WebP not found (or found unsuitable version), enabling internal WebP.")
    endif()
endif()
if(BIC_ENABLE_LIBWEBP)
    if(BIC_INTERNAL_LIBWEBP)
        if(NOT EXISTS ${BIM_LIBS_PLTFM}/libwebp.a)
            message(FATAL_ERROR "Requested to link pre-build binary libraries from '${BIM_LIBS_PLTFM}', but they do not exist. Please download and extract the optional binary libraries archive.")
        endif()
        set(WebP_INCLUDE_DIRS ${BIM_LIB_LIBWEBP}/src)
        set(WebP_LIBRARIES
            ${BIM_LIBS_PLTFM}/libwebp.a
            ${BIM_LIBS_PLTFM}/libwebpmux.a
            ${BIM_LIBS_PLTFM}/libwebpdemux.a)
    elseif(LIBBIOIMAGE)
        find_package(WebP ${BIC_LIBWEBP_REQUIRED_VERSION} REQUIRED)
    endif()
    include_directories(${WebP_INCLUDE_DIRS})
endif()



#---------------------------------------------------------------------
# Linker library interdependency:
# In order to define a matching linker order, shuffle the libraries
# here as required. They will be linked (and defined in pkg-config)
# in the order they appear here.
#---------------------------------------------------------------------
if(QT_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${QT_LIBRARIES})
endif()
if(FFTW_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${FFTW_LIBRARIES})
endif()
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${POLE_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${JZON_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${PUGIXML_LIBRARIES})
if(NIFTI_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${NIFTI_LIBRARIES})
endif()
if(FFMPEG_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${FFMPEG_LIBRARIES})
endif()
if(GDCM_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${GDCM_LIBRARIES})
endif()
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${OPENJPEG_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${LibRaw_LIBRARIES})
# to be thread-safe, better use for libraw:
#set(LINK_LIBRARIES ${LINK_LIBRARIES} ${LibRaw_r_LIBRARIES})
# NOTE: geotiff depends on proj4 so it must be listed before proj4:
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${GEOTIFF_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${PROJ4_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${TIFF_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${PNG_LIBRARY})
if(LibJpegTurbo_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${LibJpegTurbo_LIBRARIES})
endif()
if(JPEG_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${JPEG_LIBRARIES})
endif()
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${EXIV2_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${ICONV_LIBRARIES})
if(JXR_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${JXR_LIBRARIES})
endif()
if(WebP_LIBRARIES)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${WebP_LIBRARIES})
endif()
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${LCMS2_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${LIBLZMA_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${BZIP2_LIBRARIES})
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${ZLIB_LIBRARIES})
if(UNIX)
    find_package(DL REQUIRED)
    include_directories(${DL_INCLUDES})
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${DL_LIBRARIES})
elseif(MINGW)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} -lws2_32)
endif()
if(UNIX OR MINGW)
    # Find thread support (via pthread if possible):
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    #set(LINK_LIBRARIES ${LINK_LIBRARIES} Threads::Threads)
    #if(THREADS_HAVE_PTHREAD_ARG)
    #    target_compile_options(imgcnv PUBLIC "-pthread")
    #endif()
    #if(CMAKE_THREAD_LIBS_INIT)
    #    set(LINK_LIBRARIES ${LINK_LIBRARIES} "${CMAKE_THREAD_LIBS_INIT}")
    #endif()
    set(LINK_LIBRARIES ${LINK_LIBRARIES} -lpthread)
endif()



if(LIBBIOIMAGE)
    # generate a pkg-config configuration for bioimage:
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        # TODO FIXME: in the pkg-config we have absolute paths to most libraries,
        # but not i.e. to libbioimage. We could add libbioimage also like this:
        # "${LIBRARY_OUTPUT_PATH}/${CMAKE_LIBRARY_PREFIX}bioimage${CMAKE_LIBRARY_SUFFIX}"
        # instead of like this: -L${libdir} -lbioimage

        # separate libraries by space instead of semicolon:
        foreach(LIB ${LINK_LIBRARIES})
            set(BIOIMAGE_DEPENDENCY_LIBRARIES "${BIOIMAGE_DEPENDENCY_LIBRARIES} ${LIB}")
        endforeach()
        string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_INSTALL_PREFIX}/lib" BIOIMAGE_DEPENDENCY_LIBRARIES "${BIOIMAGE_DEPENDENCY_LIBRARIES}")

        # Produce a pkg-config file for linking against the shared lib
        configure_file("bioimage.pc.in" "bioimage.pc" @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/bioimage.pc" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
    endif()

    # linking the library against its dependencies is not required
    # on all architectures, but it is required i.e. on Windows:
    target_link_libraries(bioimage ${LINK_LIBRARIES})

    # Export the package for use from the build-tree
    # (this registers the build-tree with a global CMake-registry)
    export(PACKAGE bioimage)

    # Create the BioImageConfig.cmake and FooBarConfigVersion files
    #file(RELATIVE_PATH REL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}" include)
    # ... for the build tree
    set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
    configure_file(BioImageConfig.cmake.in
    "${PROJECT_BINARY_DIR}/BioImageConfig.cmake" @ONLY)
    # ... for the install tree
    #set(CONF_INCLUDE_DIRS "\${FOOBAR_CMAKE_DIR}/${REL_INCLUDE_DIR}")
    configure_file(BioImageConfig.cmake.in
    "${PROJECT_BINARY_DIR}/cmake/BioImageConfig.cmake" @ONLY)
    # ... for both
#    configure_file(FooBarConfigVersion.cmake.in
#    "${PROJECT_BINARY_DIR}/FooBarConfigVersion.cmake" @ONLY)

    # Install the BioImageConfig.cmake and FooBarConfigVersion.cmake
    install(FILES
    "${PROJECT_BINARY_DIR}/cmake/BioImageConfig.cmake" #    "${PROJECT_BINARY_DIR}/FooBarConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" COMPONENT dev)

    # Install the export set for use with the install-tree
    install(EXPORT BioImageTargets DESTINATION
    "${CMAKE_INSTALL_PREFIX}/cmake" COMPONENT dev)


endif()


if(BIC_ENABLE_IMGCNV)
    set(BIM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set(IMGCNV_SOURCES
        ${BIM_SRC}/main.cpp
        ${BIM_SRC}/reg/registration.cpp)

    # set the actual build target:
    # ${OS_BUNDLE} Expands to WIN32 or MACOS_BUNDLE depending on OS
    add_executable(imgcnv ${OS_BUNDLE} ${IMGCNV_SOURCES} ${HEADERS})
    add_dependencies(imgcnv bioimage)

    target_compile_options(imgcnv PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")
    target_compile_options(imgcnv PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")

    target_link_libraries(imgcnv bioimage ${LINK_LIBRARIES})

    # Install target
    install(TARGETS imgcnv DESTINATION bin)
endif()


#
##---------------------------------------------------------------------
## add unit tests
##---------------------------------------------------------------------
##find_package(PkgConfig)
##pkg_check_modules(GTEST REQUIRED gtest>=1.7.0)
##pkg_check_modules(GMOCK REQUIRED gmock>=1.7.0)
##
##include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
#
#enable_testing()
#
#add_executable(win32paths test/win32paths.cc)
##target_link_libraries(win32paths gtest gmock_main)
##target_link_libraries(win32paths bioimage ${LINK_LIBRARIES})
#add_test(NAME win32paths COMMAND win32paths)
#
#
##add_test(win32paths test/win32paths.cc arg1 arg2 ... )
##
###
### add_gmock_test(<target> <sources>...)
###
###  Adds a Google Mock based test executable, <target>, built from <sources> and
###  adds the test so that CTest will run it. Both the executable and the test
###  will be named <target>.
###
##function(add_gmock_test target)
##    add_executable(${target} ${ARGN})
##    target_link_libraries(${target} gmock_main)
##
##    add_test(${target} ${target})
##endfunction()
##
##
##add_gmock_test(ToDoTest ToDoTest.cc)
##target_link_libraries(ToDoTest toDoCore)
##
#
