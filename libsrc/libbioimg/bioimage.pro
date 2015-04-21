######################################################################
# Manually generated !!!
# libBioImage v 1.55 Project file
# run:
#   qmake libbioimage.pro - in order to generate Makefile for your platform
#   make all - to compile the library
#
#
# Copyright (c) 2005-2010, Bio-Image Informatic Center, UCSB
#
# To generate Makefile on any platform:
#   qmake libbioimage.pro
#
# To generate VisualStudio project file:
#   qmake -t vcapp -spec win32-msvc2005 libbioimage.pro
#   qmake -t vcapp -spec win32-msvc.net libbioimage.pro
#   qmake -t vcapp -spec win32-msvc libbioimage.pro
#   qmake -spec win32-icc libbioimage.pro # to use pure Intel Compiler
#
# To generate xcode project file:
#   qmake -spec macx-xcode libbioimage.pro
#
# To generate Makefile on MacOSX with binary install:
#   qmake -spec macx-g++ libbioimage.pro
#
######################################################################


#---------------------------------------------------------------------
# configuration: editable
#---------------------------------------------------------------------

TEMPLATE = lib
VERSION = 0.2.1

CONFIG  += staticlib

CONFIG += release
CONFIG += warn_off

# static library config

CONFIG += stat_libtiff
#CONFIG += stat_libjpeg # pick one or the other
CONFIG += stat_libjpeg_turbo # pick one or the other
CONFIG += stat_libpng
CONFIG += stat_zlib
CONFIG += ffmpeg
CONFIG += stat_exiv2
CONFIG += stat_eigen
CONFIG += libraw
#CONFIG += stat_bzlib
CONFIG += stat_libgeotiff
CONFIG += stat_proj4
CONFIG += libbioimage_transforms
CONFIG += stat_pugixml
CONFIG += stat_gdcm
#CONFIG += dyn_gdcm

CONFIG(debug, debug|release) {
  message(Building in DEBUG mode!)
  DEFINES += _DEBUG _DEBUG_
}

macx {
  QMAKE_CFLAGS_RELEASE += -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE += -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE += -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0

  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
} else:unix {
  QMAKE_CFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_CXXFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_LFLAGS_DEBUG += -pg -fPIC -ggdb

  QMAKE_CFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
}

#---------------------------------------------------------------------
# configuration paths: editable
#---------------------------------------------------------------------

BIM_SRC  = $${_PRO_FILE_PWD_}/
BIM_LSRC = $${_PRO_FILE_PWD_}/../
BIM_LIBS = $${_PRO_FILE_PWD_}/../../libs
BIM_IMGS = $${_PRO_FILE_PWD_}/../../images

HOSTTYPE = $$(HOSTTYPE)

unix {
  BIM_GENS = .generated/$$HOSTTYPE
  # path for object files
  BIM_OBJ = $$BIM_GENS/obj
  # path for generated binary
  BIM_BIN = $$BIM_GENS
}
win32 {
  BIM_GENS = ../../.generated/$(PlatformName)/$(ConfigurationName)
  # path for object files
  BIM_OBJ = $$BIM_GENS
  # path for generated binary
  BIM_BIN = ../../$(PlatformName)/$(ConfigurationName)
}


BIM_LIB_TIF    = $$BIM_LSRC/libtiff
stat_libjpeg_turbo {
  BIM_LIB_JPG  = $$BIM_LSRC/libjpeg-turbo
} else {
  BIM_LIB_JPG  = $$BIM_LSRC/libjpeg
}
BIM_LIB_PNG    = $$BIM_LSRC/libpng
BIM_LIB_Z      = $$BIM_LSRC/zlib
BIM_LIB_BZ2    = $$BIM_LSRC/bzip2
BIM_LIB_BIO    = $$BIM_LSRC/libbioimg

BIM_CORE       = $$BIM_LIB_BIO/core_lib
BIM_FMTS       = $$BIM_LIB_BIO/formats
BIM_FMTS_API   = $$BIM_LIB_BIO/formats_api
BIM_TRANSFORMS = $$BIM_LIB_BIO/transforms

ffmpeg {
  BIM_LIB_FFMPEG = $$BIM_LSRC/ffmpeg
  BIM_FMT_FFMPEG = $$BIM_FMTS/mpeg
}
BIM_LIB_EXIV2   = $$BIM_LSRC/exiv2
BIM_LIB_EIGEN   = $$BIM_LSRC/eigen
BIM_LIB_RAW     = $$BIM_LSRC/libraw
BIM_LIB_FFT     = $$BIM_LSRC/libfftw/src
BIM_LIB_GEOTIF  = $$BIM_LSRC/libgeotiff
BIM_LIB_PROJ4   = $$BIM_LSRC/proj4/src
BIM_LIB_PUGIXML = $$BIM_LSRC/pugixml/src
BIM_LIB_GDCM    = $$BIM_LSRC/gdcm
BIM_FMT_DICOM   = $$BIM_FMTS/dicom

#---------------------------------------------------------------------
# configuration: automatic
#---------------------------------------------------------------------

# enable the following only for 10.4 and universal binary generation
#macx:QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
#macx:LIBS += -faltivec -framework vecLib
#macx:CONFIG+=x86 ppc
#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4

win32 {
  DEFINES += _CRT_SECURE_NO_WARNINGS
}

BIM_LIBS_PLTFM = $$BIM_LIBS
win32: {
  BIM_LIBS_PLTFM = $$BIM_LIBS/vc2008
} else:macx {
  BIM_LIBS_PLTFM = $$BIM_LIBS/macosx
} else:unix {
  BIM_LIBS_PLTFM = $$BIM_LIBS/linux/$$HOSTTYPE
} else {
  BIM_LIBS_PLTFM = $$BIM_LIBS/linux
}

unix {
  !exists( $$BIM_GENS ) {
    message( "Cannot find directory: $$BIM_GENS, creating..." )
    system( mkdir -p $$BIM_GENS )
  }
  !exists( $$BIM_OBJ ) {
    message( "Cannot find directory: $$BIM_OBJ, creating..." )
    system( mkdir -p $$BIM_OBJ )
  }
}


#---------------------------------------------------------------------
# library configuration: automatic
#---------------------------------------------------------------------

# mac os x
macx {
  CONFIG -= stat_zlib
}


#---------------------------------------------------------------------
# generation: fixed
#---------------------------------------------------------------------

CONFIG  -= qt x11 windows

MOC_DIR = $$BIM_GENS
DESTDIR = $$BIM_BIN
OBJECTS_DIR = $$BIM_OBJ
INCLUDEPATH += $$BIM_GENS

#---------------------------------------------------------------------
# libbioimage
#---------------------------------------------------------------------

DEFINES += BIM_USE_OPENMP
#DEFINES += BIM_USE_EIGEN

INCLUDEPATH += $$BIM_LIB_BIO
INCLUDEPATH += $$BIM_FMTS_API
INCLUDEPATH += $$BIM_FMTS
INCLUDEPATH += $$BIM_FMTS/tiff
INCLUDEPATH += $$BIM_CORE

#core
SOURCES += $$BIM_CORE/xstring.cpp $$BIM_CORE/xtypes.cpp \
           $$BIM_CORE/tag_map.cpp $$BIM_CORE/xpointer.cpp $$BIM_CORE/xconf.cpp

HEADERS += $$BIM_CORE/blob_manager.h $$BIM_CORE/tag_map.h \
           $$BIM_CORE/xconf.h $$BIM_CORE/xpointer.h \
           $$BIM_CORE/xstring.h $$BIM_CORE/xtypes.h

#Formats API
SOURCES += $$BIM_FMTS_API/bim_img_format_utils.cpp \
           $$BIM_FMTS_API/bim_buffer.cpp \
           $$BIM_FMTS_API/bim_histogram.cpp \
           $$BIM_FMTS_API/bim_metatags.cpp \
           $$BIM_FMTS_API/bim_image.cpp \
           $$BIM_FMTS_API/bim_image_filters.cpp \
           $$BIM_FMTS_API/bim_image_transforms.cpp \
           $$BIM_FMTS_API/bim_image_opencv.cpp \
           $$BIM_FMTS_API/bim_image_pyramid.cpp \
           $$BIM_FMTS_API/bim_image_proxy.cpp \
           $$BIM_FMTS_API/bim_image_stack.cpp

HEADERS += $$BIM_FMTS_API/bim_buffer.h \
           $$BIM_FMTS_API/bim_histogram.h \
           $$BIM_FMTS_API/bim_image_5d.h \
	   $$BIM_FMTS_API/bim_image.h \
           $$BIM_FMTS_API/bim_image_pyramid.h \
           $$BIM_FMTS_API/bim_image_stack.h \
           $$BIM_FMTS_API/bim_img_format_interface.h \
           $$BIM_FMTS_API/bim_img_format_utils.h \
	   $$BIM_FMTS_API/bim_metatags.h \
           $$BIM_FMTS_API/bim_primitives.h \
	   $$BIM_FMTS_API/bim_qt_utils.h \
           $$BIM_FMTS_API/resize.h \
	   $$BIM_FMTS_API/rotate.h \
           $$BIM_FMTS_API/slic.h

#Formats
SOURCES += $$BIM_FMTS/bim_format_manager.cpp \
           $$BIM_FMTS/meta_format_manager.cpp \
           $$BIM_FMTS/bim_exiv_parse.cpp \
           $$BIM_FMTS/tiff/bim_tiny_tiff.cpp \
           $$BIM_FMTS/tiff/bim_tiff_format.cpp \
           $$BIM_FMTS/tiff/bim_tiff_format_io.cpp \
           $$BIM_FMTS/tiff/bim_ometiff_format_io.cpp \
           $$BIM_FMTS/tiff/bim_cz_lsm_format_io.cpp \
           $$BIM_FMTS/tiff/bim_fluoview_format_io.cpp \
           $$BIM_FMTS/tiff/bim_psia_format_io.cpp \
           $$BIM_FMTS/tiff/bim_stk_format_io.cpp \
           $$BIM_FMTS/tiff/bim_geotiff_parse.cpp \
           $$BIM_FMTS/tiff/xtiff.c \
           $$BIM_FMTS/tiff/memio.c \
           $$BIM_FMTS/jpeg/bim_jpeg_format.cpp \
           $$BIM_FMTS/biorad_pic/bim_biorad_pic_format.cpp \
           $$BIM_FMTS/bmp/bim_bmp_format.cpp \
           $$BIM_FMTS/png/bim_png_format.cpp \
           $$BIM_FMTS/nanoscope/bim_nanoscope_format.cpp \
           $$BIM_FMTS/raw/bim_raw_format.cpp \
           $$BIM_FMTS/ibw/bim_ibw_format.cpp \
           $$BIM_FMTS/ome/bim_ome_format.cpp \
           $$BIM_FMTS/ole/bim_ole_format.cpp \
           $$BIM_FMTS/ole/bim_oib_format_io.cpp \
           $$BIM_FMTS/ole/bim_zvi_format_io.cpp \
           $$BIM_FMTS/ole/zvi.cpp \
           $$BIM_FMTS/dcraw/bim_dcraw_format.cpp

HEADERS += $$BIM_FMTS/dcraw/bim_dcraw_format.h \
           $$BIM_FMTS/bmp/bim_bmp_format.h \
           $$BIM_FMTS/bim_exiv_parse.h \
           $$BIM_FMTS/ole/oib.h $$BIM_FMTS/ole/zvi.h \
           $$BIM_FMTS/ole/bim_ole_format.h \
           $$BIM_FMTS/jpeg/bim_jpeg_format.h \
           $$BIM_FMTS/png/bim_png_format.h \
           $$BIM_FMTS/raw/bim_raw_format.h \
           $$BIM_FMTS/tiff/bim_ometiff_format.h \
           $$BIM_FMTS/tiff/round.h \
           $$BIM_FMTS/tiff/bim_xtiffio.h \
           $$BIM_FMTS/tiff/bim_tiff_format.h \
           $$BIM_FMTS/tiff/bim_tiny_tiff.h \
           $$BIM_FMTS/tiff/bim_stk_format.h \
           $$BIM_FMTS/tiff/memio.h \
           $$BIM_FMTS/tiff/bim_fluoview_format.h \
           $$BIM_FMTS/tiff/bim_psia_format.h \
           $$BIM_FMTS/tiff/bim_cz_lsm_format.h \
           $$BIM_FMTS/ibw/bim_ibw_format.h \
           $$BIM_FMTS/biorad_pic/bim_biorad_pic_format.h \
           $$BIM_FMTS/bim_format_manager.h \
           $$BIM_FMTS/nanoscope/bim_nanoscope_format.h \
           $$BIM_FMTS/mpeg/parse.h \
           $$BIM_FMTS/mpeg/FfmpegCommon.h \
           $$BIM_FMTS/mpeg/IVideo.h \
           $$BIM_FMTS/mpeg/bim_ffmpeg_format.h \
           $$BIM_FMTS/mpeg/FfmpegIVideo.h \
           $$BIM_FMTS/mpeg/registry.h \
           $$BIM_FMTS/mpeg/matarray.h \
           $$BIM_FMTS/mpeg/FfmpegOVideo.h \
           $$BIM_FMTS/mpeg/debug.h \
           $$BIM_FMTS/mpeg/handle.h \
           $$BIM_FMTS/mpeg/OVideo.h \
           $$BIM_FMTS/ome/bim_ome_format.h \
           $$BIM_FMTS/meta_format_manager.h


#---------------------------------------------------------------------
# Transforms
#---------------------------------------------------------------------

DEFINES  += BIM_USE_FILTERS

libbioimage_transforms {
  DEFINES  += BIM_USE_TRANSFORMS

  macx {
    INCLUDEPATH += $$BIM_LIB_FFT/api
    LIBS += $$BIM_LIBS_PLTFM/libfftw3.a
  } else:unix {
    LIBS += -lfftw3
  }

  SOURCES += $$BIM_TRANSFORMS/chebyshev.cpp \
             $$BIM_TRANSFORMS/FuzzyCalc.cpp \
             $$BIM_TRANSFORMS/radon.cpp \
             $$BIM_TRANSFORMS/wavelet/Common.cpp \
             $$BIM_TRANSFORMS/wavelet/convolution.cpp \
             $$BIM_TRANSFORMS/wavelet/DataGrid2D.cpp \
             $$BIM_TRANSFORMS/wavelet/DataGrid3D.cpp \
             $$BIM_TRANSFORMS/wavelet/Filter.cpp \
             $$BIM_TRANSFORMS/wavelet/FilterSet.cpp \
             $$BIM_TRANSFORMS/wavelet/Symlet5.cpp \
             $$BIM_TRANSFORMS/wavelet/Wavelet.cpp \
             $$BIM_TRANSFORMS/wavelet/WaveletHigh.cpp \
             $$BIM_TRANSFORMS/wavelet/WaveletLow.cpp \
             $$BIM_TRANSFORMS/wavelet/WaveletMedium.cpp \
             $$BIM_TRANSFORMS/wavelet/wt.cpp

  HEADERS += $$BIM_FMTS/wavelet/DataGrid.h $$BIM_FMTS/wavelet/wt.h \
             $$BIM_FMTS/wavelet/DataGrid3D.h $$BIM_FMTS/wavelet/WaveletLow.h \
             $$BIM_FMTS/wavelet/Common.h $$BIM_FMTS/wavelet/convolution.h \
             $$BIM_FMTS/wavelet/WaveletHigh.h $$BIM_FMTS/wavelet/FilterSet.h \
             $$BIM_FMTS/wavelet/DataGrid2D.h $$BIM_FMTS/wavelet/Symlet5.h \
             $$BIM_FMTS/wavelet/Filter.h $$BIM_FMTS/wavelet/Wavelet.h \
             $$BIM_FMTS/wavelet/WaveletMedium.h $$BIM_FMTS/FuzzyCalc.h \
             $$BIM_FMTS/chebyshev.h $$BIM_FMTS/radon.h
}

#---------------------------------------------------------------------
# Pole
#---------------------------------------------------------------------

D_LIB_POLE = $$BIM_LSRC/pole
INCLUDEPATH += $$D_LIB_POLE
SOURCES += $$D_LIB_POLE/pole.cpp

#---------------------------------------------------------------------
# Jzon
#---------------------------------------------------------------------

D_LIB_JZON = $$BIM_LSRC/jzon
INCLUDEPATH += $$D_LIB_JZON
SOURCES += $$D_LIB_JZON/Jzon.cpp

#---------------------------------------------------------------------
# Pugixml
#---------------------------------------------------------------------

stat_pugixml {
  INCLUDEPATH += $$BIM_LIB_PUGIXML
  SOURCES += $$BIM_LIB_PUGIXML/pugixml.cpp
}

#---------------------------------------------------------------------
# ffmpeg
#---------------------------------------------------------------------

ffmpeg {
  DEFINES  += BIM_FFMPEG_FORMAT FFMPEG_VIDEO_DISABLE_MATLAB __STDC_CONSTANT_MACROS
  INCLUDEPATH += $$BIM_LIB_FFMPEG/include
  win32:INCLUDEPATH += $$BIM_LIB_FFMPEG/include-win

  SOURCES += $$BIM_FMT_FFMPEG/debug.cpp $$BIM_FMT_FFMPEG/bim_ffmpeg_format.cpp \
             $$BIM_FMT_FFMPEG/FfmpegCommon.cpp $$BIM_FMT_FFMPEG/FfmpegIVideo.cpp \
             $$BIM_FMT_FFMPEG/FfmpegOVideo.cpp $$BIM_FMT_FFMPEG/registry.cpp

#  win32 {
#    LIBS += $$BIM_LIBS_PLTFM/avcodec.lib
#    LIBS += $$BIM_LIBS_PLTFM/avformat.lib
#    LIBS += $$BIM_LIBS_PLTFM/avutil.lib
#    LIBS += $$BIM_LIBS_PLTFM/swscale.lib
#  } else:macx {
#    LIBS += $$BIM_LIBS_PLTFM/libavformat.a
#    LIBS += $$BIM_LIBS_PLTFM/libavcodec.a
#    LIBS += $$BIM_LIBS_PLTFM/libswscale.a
#    LIBS += $$BIM_LIBS_PLTFM/libavutil.a
#    LIBS += $$BIM_LIBS_PLTFM/libvpx.a
#    LIBS += $$BIM_LIBS_PLTFM/libx264.a
#    LIBS += $$BIM_LIBS_PLTFM/libx264.a
#    LIBS += $$BIM_LIBS_PLTFM/libvpx.a
#    LIBS += $$BIM_LIBS_PLTFM/libxvidcore.a
#    LIBS += $$BIM_LIBS_PLTFM/libogg.a
#    LIBS += $$BIM_LIBS_PLTFM/libtheora.a
#    LIBS += $$BIM_LIBS_PLTFM/libtheoraenc.a
#    LIBS += $$BIM_LIBS_PLTFM/libtheoradec.a
#    LIBS += -framework CoreFoundation -framework VideoDecodeAcceleration -framework QuartzCore
#  } else:unix {
#    LIBS += $$BIM_LIBS_PLTFM/libavformat.a
#    LIBS += $$BIM_LIBS_PLTFM/libavcodec.a
#    LIBS += $$BIM_LIBS_PLTFM/libswscale.a
#    LIBS += $$BIM_LIBS_PLTFM/libavutil.a
#    LIBS += $$BIM_LIBS_PLTFM/libvpx.a
#    LIBS += $$BIM_LIBS_PLTFM/libx264.a
#
#    LIBS += -lpthread -lxvidcore -lopenjpeg -lschroedinger-1.0 -ltheora -ltheoraenc -ltheoradec -lbz2
#  }
} # FFMPEG

#---------------------------------------------------------------------
# GDCM - under linux we only use system dynamic version right now
#---------------------------------------------------------------------

stat_gdcm {
  DEFINES += BIM_GDCM_FORMAT
  SOURCES += $$BIM_FMT_DICOM/bim_dicom_format.cpp

  win32 {
    INCLUDEPATH += $$BIM_LIB_GDCM/projects/win64
  } else {
    INCLUDEPATH += $$BIM_LIB_GDCM/projects/unix
  }

  INCLUDEPATH += $$BIM_LIB_GDCM/Source/Common
  INCLUDEPATH += $$BIM_LIB_GDCM/Source/DataDictionary
  INCLUDEPATH += $$BIM_LIB_GDCM/Source/DataStructureAndEncodingDefinition
  INCLUDEPATH += $$BIM_LIB_GDCM/Source/InformationObjectDefinition
  INCLUDEPATH += $$BIM_LIB_GDCM/Source/MediaStorageAndFileFormat

#  win32 {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmcharls.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmCommon.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmDICT.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmDSED.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmexpat.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmgetopt.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmjpeg12.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmjpeg16.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmjpeg8.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmMEXD.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmMSFF.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmopenjpeg.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmzlib.lib
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/socketxx.lib
#  } else {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmDICT.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmMSFF.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmCommon.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmDSED.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmIOD.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmcharls.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmexpat.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmjpeg8.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmjpeg12.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmjpeg16.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmopenjpeg.a
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmzlib.a
#  }
} # static GDCM


dyn_gdcm {
  DEFINES += BIM_GDCM_FORMAT
  SOURCES += $$BIM_FMT_DICOM/bim_dicom_format.cpp

#  win32 {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.lib
#  } else:macx {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.a
#  } else:unix {
#    LIBS += -lgdcm
#  }
} # System GDCM

#---------------------------------------------------------------------
# libraw
#---------------------------------------------------------------------

libraw {
  DEFINES  += LIBRAW_BUILDLIB LIBRAW_NODLL USE_JPEG
  INCLUDEPATH += $$BIM_LIB_RAW

  SOURCES += $$BIM_LIB_RAW/src/libraw_c_api.cpp \
             $$BIM_LIB_RAW/src/libraw_cxx.cpp \
             $$BIM_LIB_RAW/src/libraw_datastream.cpp \
             $$BIM_LIB_RAW/internal/demosaic_packs.cpp \
             $$BIM_LIB_RAW/internal/dcraw_fileio.cpp \
             $$BIM_LIB_RAW/internal/dcraw_common.cpp

  HEADERS += $$BIM_FMTS/internal/libraw_internal_funcs.h \
             $$BIM_FMTS/internal/var_defines.h \
             $$BIM_FMTS/internal/defines.h
}

#---------------------------------------------------------------------
# Now adding static libraries
#---------------------------------------------------------------------

#some configs first
unix:DEFINES  += HAVE_UNISTD_H
unix:DEFINES  -= HAVE_IO_H
win32:DEFINES += HAVE_IO_H

macx:DEFINES  += HAVE_UNISTD_H
#macx:DEFINES  += WORDS_BIGENDIAN
macx:DEFINES  -= HAVE_IO_H

#---------------------------------------------------------------------
# libTiff
#---------------------------------------------------------------------

stat_libtiff {
  INCLUDEPATH += $$BIM_LIB_TIF
  SOURCES += $$BIM_LIB_TIF/tif_fax3sm.c $$BIM_LIB_TIF/tif_aux.c \
             $$BIM_LIB_TIF/tif_close.c $$BIM_LIB_TIF/tif_codec.c \
             $$BIM_LIB_TIF/tif_color.c $$BIM_LIB_TIF/tif_compress.c \
             $$BIM_LIB_TIF/tif_dir.c $$BIM_LIB_TIF/tif_dirinfo.c \
             $$BIM_LIB_TIF/tif_dirread.c $$BIM_LIB_TIF/tif_dirwrite.c \
             $$BIM_LIB_TIF/tif_dumpmode.c $$BIM_LIB_TIF/tif_error.c \
             $$BIM_LIB_TIF/tif_extension.c $$BIM_LIB_TIF/tif_fax3.c \
             $$BIM_LIB_TIF/tif_flush.c $$BIM_LIB_TIF/tif_getimage.c \
             $$BIM_LIB_TIF/tif_jpeg.c $$BIM_LIB_TIF/tif_luv.c \
             $$BIM_LIB_TIF/tif_lzw.c $$BIM_LIB_TIF/tif_next.c \
             $$BIM_LIB_TIF/tif_open.c $$BIM_LIB_TIF/tif_packbits.c \
             $$BIM_LIB_TIF/tif_pixarlog.c $$BIM_LIB_TIF/tif_predict.c \
             $$BIM_LIB_TIF/tif_print.c $$BIM_LIB_TIF/tif_read.c \
             $$BIM_LIB_TIF/tif_strip.c $$BIM_LIB_TIF/tif_swab.c \
             $$BIM_LIB_TIF/tif_thunder.c $$BIM_LIB_TIF/tif_tile.c \
             $$BIM_LIB_TIF/tif_version.c $$BIM_LIB_TIF/tif_warning.c \
             $$BIM_LIB_TIF/tif_write.c $$BIM_LIB_TIF/tif_zip.c

  unix:SOURCES  += $$BIM_LIB_TIF/tif_unix.c
  win32:SOURCES += $$BIM_LIB_TIF/tif_win32.c
}

#---------------------------------------------------------------------
# libGeoTiff
#---------------------------------------------------------------------

stat_libgeotiff {
  INCLUDEPATH += $$BIM_LIB_GEOTIF
  SOURCES += $$BIM_LIB_GEOTIF/cpl_csv.c $$BIM_LIB_GEOTIF/cpl_serv.c \
             $$BIM_LIB_GEOTIF/geotiff_proj4.c $$BIM_LIB_GEOTIF/geo_extra.c \
             $$BIM_LIB_GEOTIF/geo_free.c $$BIM_LIB_GEOTIF/geo_get.c \
             $$BIM_LIB_GEOTIF/geo_names.c $$BIM_LIB_GEOTIF/geo_new.c \
             $$BIM_LIB_GEOTIF/geo_normalize.c $$BIM_LIB_GEOTIF/geo_print.c \
             $$BIM_LIB_GEOTIF/geo_set.c $$BIM_LIB_GEOTIF/geo_simpletags.c \
             $$BIM_LIB_GEOTIF/geo_strtod.c $$BIM_LIB_GEOTIF/geo_tiffp.c \
             $$BIM_LIB_GEOTIF/geo_trans.c $$BIM_LIB_GEOTIF/geo_write.c
} else {
  unix:LIBS += -lgeotiff
  win32:LIBS += $$BIM_LIBS_PLTFM/libgeotiff.lib
}

#---------------------------------------------------------------------
# Proj.4
#---------------------------------------------------------------------

stat_proj4 {
  INCLUDEPATH += $$BIM_LIB_PROJ4
  SOURCES += $$BIM_LIB_PROJ4/aasincos.c $$BIM_LIB_PROJ4/adjlon.c $$BIM_LIB_PROJ4/bch2bps.c \
              $$BIM_LIB_PROJ4/bchgen.c $$BIM_LIB_PROJ4/biveval.c $$BIM_LIB_PROJ4/dmstor.c \
              $$BIM_LIB_PROJ4/emess.c $$BIM_LIB_PROJ4/gen_cheb.c $$BIM_LIB_PROJ4/geocent.c \
              $$BIM_LIB_PROJ4/geod_for.c $$BIM_LIB_PROJ4/geod_inv.c $$BIM_LIB_PROJ4/geod_set.c \
              $$BIM_LIB_PROJ4/jniproj.c $$BIM_LIB_PROJ4/mk_cheby.c $$BIM_LIB_PROJ4/nad_cvt.c \
              $$BIM_LIB_PROJ4/nad_init.c $$BIM_LIB_PROJ4/nad_intr.c $$BIM_LIB_PROJ4/PJ_aea.c \
              $$BIM_LIB_PROJ4/PJ_aeqd.c $$BIM_LIB_PROJ4/PJ_airy.c $$BIM_LIB_PROJ4/PJ_aitoff.c \
              $$BIM_LIB_PROJ4/pj_apply_gridshift.c $$BIM_LIB_PROJ4/pj_apply_vgridshift.c $$BIM_LIB_PROJ4/PJ_august.c \
              $$BIM_LIB_PROJ4/pj_auth.c $$BIM_LIB_PROJ4/PJ_bacon.c $$BIM_LIB_PROJ4/PJ_bipc.c \
              $$BIM_LIB_PROJ4/PJ_boggs.c $$BIM_LIB_PROJ4/PJ_bonne.c $$BIM_LIB_PROJ4/PJ_cass.c \
              $$BIM_LIB_PROJ4/PJ_cc.c $$BIM_LIB_PROJ4/PJ_cea.c $$BIM_LIB_PROJ4/PJ_chamb.c \
              $$BIM_LIB_PROJ4/PJ_collg.c $$BIM_LIB_PROJ4/PJ_crast.c $$BIM_LIB_PROJ4/pj_ctx.c \
              $$BIM_LIB_PROJ4/pj_datums.c $$BIM_LIB_PROJ4/pj_datum_set.c $$BIM_LIB_PROJ4/PJ_denoy.c \
              $$BIM_LIB_PROJ4/pj_deriv.c $$BIM_LIB_PROJ4/PJ_eck1.c $$BIM_LIB_PROJ4/PJ_eck2.c \
              $$BIM_LIB_PROJ4/PJ_eck3.c $$BIM_LIB_PROJ4/PJ_eck4.c $$BIM_LIB_PROJ4/PJ_eck5.c \
              $$BIM_LIB_PROJ4/pj_ellps.c $$BIM_LIB_PROJ4/pj_ell_set.c $$BIM_LIB_PROJ4/PJ_eqc.c \
              $$BIM_LIB_PROJ4/PJ_eqdc.c $$BIM_LIB_PROJ4/pj_errno.c $$BIM_LIB_PROJ4/pj_factors.c \
              $$BIM_LIB_PROJ4/PJ_fahey.c $$BIM_LIB_PROJ4/PJ_fouc_s.c $$BIM_LIB_PROJ4/pj_fwd.c \
              $$BIM_LIB_PROJ4/PJ_gall.c $$BIM_LIB_PROJ4/pj_gauss.c $$BIM_LIB_PROJ4/pj_geocent.c \
              $$BIM_LIB_PROJ4/PJ_geos.c $$BIM_LIB_PROJ4/PJ_gins8.c $$BIM_LIB_PROJ4/PJ_gnom.c \
              $$BIM_LIB_PROJ4/PJ_gn_sinu.c $$BIM_LIB_PROJ4/PJ_goode.c $$BIM_LIB_PROJ4/pj_gridinfo.c \
              $$BIM_LIB_PROJ4/pj_gridlist.c $$BIM_LIB_PROJ4/PJ_gstmerc.c $$BIM_LIB_PROJ4/PJ_hammer.c \
              $$BIM_LIB_PROJ4/PJ_hatano.c $$BIM_LIB_PROJ4/PJ_healpix.c $$BIM_LIB_PROJ4/PJ_igh.c \
              $$BIM_LIB_PROJ4/PJ_imw_p.c $$BIM_LIB_PROJ4/pj_init.c $$BIM_LIB_PROJ4/pj_initcache.c \
              $$BIM_LIB_PROJ4/pj_inv.c $$BIM_LIB_PROJ4/PJ_isea.c $$BIM_LIB_PROJ4/PJ_krovak.c \
              $$BIM_LIB_PROJ4/PJ_labrd.c $$BIM_LIB_PROJ4/PJ_laea.c $$BIM_LIB_PROJ4/PJ_lagrng.c \
              $$BIM_LIB_PROJ4/PJ_larr.c $$BIM_LIB_PROJ4/PJ_lask.c $$BIM_LIB_PROJ4/pj_latlong.c \
              $$BIM_LIB_PROJ4/PJ_lcc.c $$BIM_LIB_PROJ4/PJ_lcca.c $$BIM_LIB_PROJ4/pj_list.c \
              $$BIM_LIB_PROJ4/pj_log.c $$BIM_LIB_PROJ4/PJ_loxim.c $$BIM_LIB_PROJ4/PJ_lsat.c \
              $$BIM_LIB_PROJ4/pj_malloc.c $$BIM_LIB_PROJ4/PJ_mbtfpp.c $$BIM_LIB_PROJ4/PJ_mbtfpq.c \
              $$BIM_LIB_PROJ4/PJ_mbt_fps.c $$BIM_LIB_PROJ4/PJ_merc.c $$BIM_LIB_PROJ4/PJ_mill.c \
              $$BIM_LIB_PROJ4/pj_mlfn.c $$BIM_LIB_PROJ4/PJ_mod_ster.c $$BIM_LIB_PROJ4/PJ_moll.c \
              $$BIM_LIB_PROJ4/pj_msfn.c $$BIM_LIB_PROJ4/pj_mutex.c $$BIM_LIB_PROJ4/PJ_natearth.c \
              $$BIM_LIB_PROJ4/PJ_nell.c $$BIM_LIB_PROJ4/PJ_nell_h.c $$BIM_LIB_PROJ4/PJ_nocol.c \
              $$BIM_LIB_PROJ4/PJ_nsper.c $$BIM_LIB_PROJ4/PJ_nzmg.c $$BIM_LIB_PROJ4/PJ_ob_tran.c \
              $$BIM_LIB_PROJ4/PJ_ocea.c $$BIM_LIB_PROJ4/PJ_oea.c $$BIM_LIB_PROJ4/PJ_omerc.c \
              $$BIM_LIB_PROJ4/pj_open_lib.c $$BIM_LIB_PROJ4/PJ_ortho.c $$BIM_LIB_PROJ4/pj_param.c \
              $$BIM_LIB_PROJ4/pj_phi2.c $$BIM_LIB_PROJ4/PJ_poly.c $$BIM_LIB_PROJ4/pj_pr_list.c \
              $$BIM_LIB_PROJ4/PJ_putp2.c $$BIM_LIB_PROJ4/PJ_putp3.c $$BIM_LIB_PROJ4/PJ_putp4p.c \
              $$BIM_LIB_PROJ4/PJ_putp5.c $$BIM_LIB_PROJ4/PJ_putp6.c $$BIM_LIB_PROJ4/pj_qsfn.c \
              $$BIM_LIB_PROJ4/pj_release.c $$BIM_LIB_PROJ4/PJ_robin.c $$BIM_LIB_PROJ4/PJ_rpoly.c \
              $$BIM_LIB_PROJ4/PJ_sconics.c $$BIM_LIB_PROJ4/PJ_somerc.c $$BIM_LIB_PROJ4/PJ_stere.c \
              $$BIM_LIB_PROJ4/PJ_sterea.c $$BIM_LIB_PROJ4/pj_strerrno.c $$BIM_LIB_PROJ4/PJ_sts.c \
              $$BIM_LIB_PROJ4/PJ_tcc.c $$BIM_LIB_PROJ4/PJ_tcea.c $$BIM_LIB_PROJ4/PJ_tmerc.c \
              $$BIM_LIB_PROJ4/PJ_tpeqd.c $$BIM_LIB_PROJ4/pj_transform.c $$BIM_LIB_PROJ4/pj_tsfn.c \
              $$BIM_LIB_PROJ4/pj_units.c $$BIM_LIB_PROJ4/PJ_urm5.c $$BIM_LIB_PROJ4/PJ_urmfps.c \
              $$BIM_LIB_PROJ4/pj_utils.c $$BIM_LIB_PROJ4/PJ_vandg.c $$BIM_LIB_PROJ4/PJ_vandg2.c \
              $$BIM_LIB_PROJ4/PJ_vandg4.c $$BIM_LIB_PROJ4/PJ_wag2.c $$BIM_LIB_PROJ4/PJ_wag3.c \
              $$BIM_LIB_PROJ4/PJ_wag7.c $$BIM_LIB_PROJ4/PJ_wink1.c $$BIM_LIB_PROJ4/PJ_wink2.c \
              $$BIM_LIB_PROJ4/pj_zpoly1.c $$BIM_LIB_PROJ4/proj.c $$BIM_LIB_PROJ4/proj_etmerc.c \
              $$BIM_LIB_PROJ4/proj_mdist.c $$BIM_LIB_PROJ4/proj_rouss.c $$BIM_LIB_PROJ4/p_series.c \
              $$BIM_LIB_PROJ4/rtodms.c $$BIM_LIB_PROJ4/vector1.c
} else {
  unix:LIBS += -lproj4
  win32:LIBS += $$BIM_LIBS_PLTFM/proj4.lib
}

#---------------------------------------------------------------------
# libPng
#---------------------------------------------------------------------

stat_libpng {
  INCLUDEPATH += $$BIM_LIB_PNG

  # by default disable intel asm code
  unix:DEFINES += PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD

  win32:DEFINES -= PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD
  macx:DEFINES -= PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD
  linux-g++-32:DEFINES -= PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD

  SOURCES += $$BIM_LIB_PNG/png.c $$BIM_LIB_PNG/pngerror.c $$BIM_LIB_PNG/pngget.c \
             $$BIM_LIB_PNG/pngmem.c $$BIM_LIB_PNG/pngpread.c $$BIM_LIB_PNG/pngread.c \
             $$BIM_LIB_PNG/pngrio.c $$BIM_LIB_PNG/pngrtran.c $$BIM_LIB_PNG/pngrutil.c \
             $$BIM_LIB_PNG/pngset.c $$BIM_LIB_PNG/pngtrans.c \
             $$BIM_LIB_PNG/pngwio.c $$BIM_LIB_PNG/pngwrite.c $$BIM_LIB_PNG/pngwtran.c \
             $$BIM_LIB_PNG/pngwutil.c
} else {
#  unix:LIBS += -lpng
#  win32:LIBS += $$BIM_LIBS_PLTFM/libpng.lib
}


#---------------------------------------------------------------------
# ZLib
#---------------------------------------------------------------------

stat_zlib {
  INCLUDEPATH += $$BIM_LIB_Z

  SOURCES += $$BIM_LIB_Z/adler32.c $$BIM_LIB_Z/compress.c $$BIM_LIB_Z/crc32.c \
             $$BIM_LIB_Z/deflate.c $$BIM_LIB_Z/infback.c \
             $$BIM_LIB_Z/inffast.c $$BIM_LIB_Z/inflate.c $$BIM_LIB_Z/inftrees.c \
             $$BIM_LIB_Z/trees.c $$BIM_LIB_Z/uncompr.c $$BIM_LIB_Z/zutil.c
} else {
#    unix:LIBS += -lz
#    win32:LIBS += $$BIM_LIBS_PLTFM/zlib.lib
}

#---------------------------------------------------------------------
# libjpeg
#---------------------------------------------------------------------

stat_libjpeg {
  INCLUDEPATH += $$BIM_LIB_JPG
  SOURCES += $$BIM_LIB_JPG/jaricom.c $$BIM_LIB_JPG/jcapimin.c $$BIM_LIB_JPG/jcapistd.c \
             $$BIM_LIB_JPG/jcarith.c $$BIM_LIB_JPG/jccoefct.c \
             $$BIM_LIB_JPG/jccolor.c $$BIM_LIB_JPG/jcdctmgr.c $$BIM_LIB_JPG/jchuff.c \
             $$BIM_LIB_JPG/jcinit.c $$BIM_LIB_JPG/jcmainct.c $$BIM_LIB_JPG/jcmarker.c \
             $$BIM_LIB_JPG/jcmaster.c $$BIM_LIB_JPG/jcomapi.c $$BIM_LIB_JPG/jcparam.c \
             $$BIM_LIB_JPG/jcprepct.c $$BIM_LIB_JPG/jcsample.c $$BIM_LIB_JPG/jctrans.c \
             $$BIM_LIB_JPG/jdapimin.c $$BIM_LIB_JPG/jdapistd.c $$BIM_LIB_JPG/jdarith.c \
             $$BIM_LIB_JPG/jdatadst.c $$BIM_LIB_JPG/jdatasrc.c $$BIM_LIB_JPG/jdcoefct.c \
             $$BIM_LIB_JPG/jdcolor.c $$BIM_LIB_JPG/jddctmgr.c $$BIM_LIB_JPG/jdhuff.c \
             $$BIM_LIB_JPG/jdinput.c $$BIM_LIB_JPG/jdmainct.c $$BIM_LIB_JPG/jdmarker.c \
             $$BIM_LIB_JPG/jdmaster.c $$BIM_LIB_JPG/jdmerge.c \
             $$BIM_LIB_JPG/jdpostct.c $$BIM_LIB_JPG/jdsample.c $$BIM_LIB_JPG/jdtrans.c \
             $$BIM_LIB_JPG/jerror.c $$BIM_LIB_JPG/jfdctflt.c $$BIM_LIB_JPG/jfdctfst.c \
             $$BIM_LIB_JPG/jfdctint.c $$BIM_LIB_JPG/jidctflt.c $$BIM_LIB_JPG/jidctfst.c \
             $$BIM_LIB_JPG/jidctint.c $$BIM_LIB_JPG/jmemmgr.c \
             $$BIM_LIB_JPG/jquant1.c $$BIM_LIB_JPG/jquant2.c $$BIM_LIB_JPG/jutils.c \
             $$BIM_LIB_JPG/jmemansi.c
} else {
#  unix:LIBS += -ljpeg
#  win32:LIBS += $$BIM_LIBS_PLTFM/libjpeg.lib
}

#---------------------------------------------------------------------
# libjpeg-turbo
#---------------------------------------------------------------------

stat_libjpeg_turbo {
  INCLUDEPATH += $$BIM_LIB_JPG
  #SOURCES += $$BIM_LIB_JPG/jaricom.c $$BIM_LIB_JPG/jcapimin.c $$BIM_LIB_JPG/jcapistd.c
  win32 {
    LIBS += $$BIM_LIBS_PLTFM/turbojpeg-static.lib
  } else {
    LIBS += $$BIM_LIBS_PLTFM/libjpeg.a
  }
}

dyn_libjpeg_turbo {
  win32 {
    LIBS += $$BIM_LIBS_PLTFM/turbojpeg-static.lib
  } else {
    LIBS += -lturbojpeg
  }
}

#---------------------------------------------------------------------
# bzlib
#---------------------------------------------------------------------

stat_bzlib {
  INCLUDEPATH += $$BIM_LIB_BZ2
  SOURCES += $$BIM_LIB_BZ2/blocksort.c $$BIM_LIB_BZ2/bzlib.c $$BIM_LIB_BZ2/randtable.c \
             $$BIM_LIB_BZ2/compress.c $$BIM_LIB_BZ2/crctable.c $$BIM_LIB_BZ2/decompress.c \
             $$BIM_LIB_BZ2/huffman.c
} else {
#  unix:LIBS += -lbz2
#  macx:LIBS += -lbz2
#  #win32:LIBS += $$BIM_LIBS_PLTFM/libbz2.lib
}

#---------------------------------------------------------------------
# exiv2
#---------------------------------------------------------------------

stat_exiv2 {
  DEFINES += SUPPRESS_WARNINGS
  INCLUDEPATH += $$BIM_LIB_EXIV2
  INCLUDEPATH += $$BIM_LIB_EXIV2/exiv2
  SOURCES += $$BIM_LIB_EXIV2/exiv2/asfvideo.cpp $$BIM_LIB_EXIV2/exiv2/basicio.cpp \
             $$BIM_LIB_EXIV2/exiv2/bmpimage.cpp $$BIM_LIB_EXIV2/exiv2/canonmn.cpp \
             $$BIM_LIB_EXIV2/exiv2/convert.cpp $$BIM_LIB_EXIV2/exiv2/cr2image.cpp \
             $$BIM_LIB_EXIV2/exiv2/crwimage.cpp $$BIM_LIB_EXIV2/exiv2/datasets.cpp \
             $$BIM_LIB_EXIV2/exiv2/easyaccess.cpp $$BIM_LIB_EXIV2/exiv2/epsimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/error.cpp $$BIM_LIB_EXIV2/exiv2/exif.cpp \
             $$BIM_LIB_EXIV2/exiv2/fujimn.cpp $$BIM_LIB_EXIV2/exiv2/futils.cpp \
             $$BIM_LIB_EXIV2/exiv2/gifimage.cpp $$BIM_LIB_EXIV2/exiv2/image.cpp \
             $$BIM_LIB_EXIV2/exiv2/iptc.cpp $$BIM_LIB_EXIV2/exiv2/jp2image.cpp \
             $$BIM_LIB_EXIV2/exiv2/jpgimage.cpp $$BIM_LIB_EXIV2/exiv2/makernote.cpp \
             $$BIM_LIB_EXIV2/exiv2/matroskavideo.cpp $$BIM_LIB_EXIV2/exiv2/metadatum.cpp \
             $$BIM_LIB_EXIV2/exiv2/minoltamn.cpp $$BIM_LIB_EXIV2/exiv2/mrwimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/nikonmn.cpp $$BIM_LIB_EXIV2/exiv2/olympusmn.cpp \
             $$BIM_LIB_EXIV2/exiv2/orfimage.cpp $$BIM_LIB_EXIV2/exiv2/panasonicmn.cpp \
             $$BIM_LIB_EXIV2/exiv2/pentaxmn.cpp $$BIM_LIB_EXIV2/exiv2/pgfimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/pngchunk.cpp $$BIM_LIB_EXIV2/exiv2/pngimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/preview.cpp $$BIM_LIB_EXIV2/exiv2/properties.cpp \
             $$BIM_LIB_EXIV2/exiv2/quicktimevideo.cpp $$BIM_LIB_EXIV2/exiv2/psdimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/rafimage.cpp $$BIM_LIB_EXIV2/exiv2/riffvideo.cpp \
             $$BIM_LIB_EXIV2/exiv2/rw2image.cpp $$BIM_LIB_EXIV2/exiv2/sigmamn.cpp \
             $$BIM_LIB_EXIV2/exiv2/samsungmn.cpp $$BIM_LIB_EXIV2/exiv2/sonymn.cpp \
             $$BIM_LIB_EXIV2/exiv2/tags.cpp $$BIM_LIB_EXIV2/exiv2/tgaimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/tiffcomposite.cpp $$BIM_LIB_EXIV2/exiv2/tiffimage.cpp \
             $$BIM_LIB_EXIV2/exiv2/tiffvisitor.cpp $$BIM_LIB_EXIV2/exiv2/types.cpp \
             $$BIM_LIB_EXIV2/exiv2/utils.cpp $$BIM_LIB_EXIV2/exiv2/value.cpp \
             $$BIM_LIB_EXIV2/exiv2/version.cpp $$BIM_LIB_EXIV2/exiv2/xmp.cpp \
             $$BIM_LIB_EXIV2/exiv2/xmpsidecar.cpp
} else {
#  unix:LIBS += -lexiv2
#  win32:LIBS += $$BIM_LIBS_PLTFM/libexiv2.lib
}

#---------------------------------------------------------------------
#eigen
#---------------------------------------------------------------------

stat_eigen {
  INCLUDEPATH += $$BIM_LIB_EIGEN
}

