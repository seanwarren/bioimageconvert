######################################################################
# Manually generated !!!
# BioImageConvert v 1.50 Project file
# run: qmake imgcnv.pro in order to generate Makefile for your platform
# Copyright (c) 2005-2010, Bio-Image Informatic Center, UCSB
#
# To generate Makefile on any platform:
#   qmake imgcnv.pro
#
# To generate VisualStudio project file:
#   qmake -t vcapp -spec win32-msvc2005 imgcnv.pro
#   qmake -t vcapp -spec win32-msvc.net imgcnv.pro
#   qmake -t vcapp -spec win32-msvc imgcnv.pro
#   qmake -spec win32-icc imgcnv.pro # to use pure Intel Compiler
#
# To generate xcode project file:
#   qmake -spec macx-xcode imgcnv.pro
#
# To generate Makefile on MacOSX with binary install:
#   qmake -spec macx-g++ imgcnv.pro
#
######################################################################

#---------------------------------------------------------------------
# configuration: editable
#---------------------------------------------------------------------
APP_NAME = imgcnv
TARGET = imgcnv

TEMPLATE = lib
CONFIG  += dll
VERSION = 2.0.9

CONFIG += console

CONFIG += release
#CONFIG += debug
CONFIG += warn_off

# static library config
CONFIG += stat_libtiff
#CONFIG += stat_libjpeg
CONFIG += stat_libjpeg_turbo # pick one or the other
CONFIG += stat_libpng
CONFIG += stat_zlib
CONFIG += ffmpeg
CONFIG += sys_bzlib
CONFIG += stat_exiv2
CONFIG += stat_eigen
CONFIG += libraw
CONFIG += stat_gdcm
#CONFIG += dyn_gdcm
CONFIG += stat_openjpeg
CONFIG += stat_jxrlib
CONFIG += stat_libwebp
CONFIG += stat_lcms2

CONFIG += dyn_lzma

CONFIG(debug, debug|release) {
   message(Building in DEBUG mode!)
   DEFINES += _DEBUG _DEBUG_
}

macx {
  QMAKE_CFLAGS_RELEASE = -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE = -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE = -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
} else:unix {
  QMAKE_CFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_CXXFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_LFLAGS_DEBUG += -Wl,-Bsymbolic -pg -fPIC -ggdb

  QMAKE_CFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE += -Wl,-Bsymbolic -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
}


#---------------------------------------------------------------------
# configuration paths: editable
#---------------------------------------------------------------------

BIM_SRC = ../src
DN_LSRC = ../libsrc
DN_LIBS = ../libs
DN_IMGS = ../images

HOSTTYPE = $$(HOSTTYPE)

unix {
  DN_GENS = ../.generated/$$HOSTTYPE
  # path for object files
  DN_OBJ = $$DN_GENS/obj
  # path for generated binary
  DN_BIN = ../$$HOSTTYPE
}
win32 {
  DN_GENS = ../.generated/$(PlatformName)/$(ConfigurationName)
  # path for object files
  DN_OBJ = $$DN_GENS
  # path for generated binary
  DN_BIN = ../$(PlatformName)/$(ConfigurationName)
}

DN_LIB_TIF = $$DN_LSRC/libtiff
DN_LIB_JPG = $$DN_LSRC/libjpeg
DN_LIB_PNG = $$DN_LSRC/libpng
DN_LIB_Z   = $$DN_LSRC/zlib
DN_LIB_BZ2 = $$DN_LSRC/bzip2
DN_LIB_BIO = $$DN_LSRC/libbioimg

DN_CORE     = $$DN_LIB_BIO/core_lib
DN_FMTS     = $$DN_LIB_BIO/formats
DN_FMTS_API = $$DN_LIB_BIO/formats_api

ffmpeg {
  DN_LIB_FFMPEG = $$DN_LSRC/ffmpeg
  DN_FMT_FFMPEG = $$DN_FMTS/mpeg
}
DN_LIB_EXIV2   = $$DN_LSRC/exiv2
DN_LIB_EIGEN   = $$DN_LSRC/eigen
DN_LIB_RAW     = $$DN_LSRC/libraw
BIM_LIB_GDCM   = $$DN_LSRC/gdcm

#---------------------------------------------------------------------
# configuration: automatic
#---------------------------------------------------------------------

win32 {
  DEFINES += _CRT_SECURE_NO_WARNINGS
}

win32: {
  BIM_LIBS_PLTFM = $$DN_LIBS/vc2008
} else:macx {
  BIM_LIBS_PLTFM = $$DN_LIBS/macosx
} else:unix {
  BIM_LIBS_PLTFM = $$DN_LIBS/linux/$$HOSTTYPE
} else {
  BIM_LIBS_PLTFM = $$DN_LIBS/linux
}

unix {
  !exists( $$DN_GENS ) {
    message( "Cannot find directory: $$DN_GENS, creating..." )
    system( mkdir -p $$DN_GENS )
  }
  !exists( $$DN_OBJ ) {
    message( "Cannot find directory: $$DN_OBJ, creating..." )
    system( mkdir -p $$DN_OBJ )
  }
}

win32 {
  !exists( $$DN_GENS ) {
    message( "Cannot find directory: $$DN_GENS, creating..." )
    !exists( ../.generated/Win32/Debug       ) { system( mkdir ..\\.generated\\Win32\\Debug ) }
    !exists( ../.generated/Win32/Release     ) { system( mkdir ..\\.generated\\Win32\\Release ) }
    !exists( ../.generated/Win32/ICC_Release ) { system( mkdir ..\\.generated\\Win32\\ICC_Release ) }
    !exists( ../.generated/x64/Debug         ) { system( mkdir ..\\.generated\\x64\\Debug ) }
    !exists( ../.generated/x64/Release       ) { system( mkdir ..\\.generated\\x64\\Release ) }
    !exists( ../.generated/x64/ICC_Release   ) { system( mkdir ..\\.generated\\x64\\ICC_Release ) }
  }
  !exists( $$DN_OBJ ) {
    message( "Cannot find directory: $$DN_GENS, creating..." )
    !exists( ../.generated/Win32/Debug   ) { system( mkdir ..\\.generated\\Win32\\Debug ) }
    !exists( ../.generated/Win32/Release ) { system( mkdir ..\\.generated\\Win32\\Release ) }
    !exists( ../.generated/x64/Debug     ) { system( mkdir ..\\.generated\\x64\\Debug ) }
    !exists( ../.generated/x64/Release   ) { system( mkdir ..\\.generated\\x64\\Release ) }
  }
}

#---------------------------------------------------------------------
# generation: required
#---------------------------------------------------------------------

CONFIG  -= qt x11 windows

MOC_DIR = $$DN_GENS
DESTDIR = $$DN_BIN
OBJECTS_DIR = $$DN_OBJ
INCLUDEPATH += $$DN_GENS

#---------------------------------------------------------------------
# main sources
#---------------------------------------------------------------------

SOURCES += $$BIM_SRC/main.cpp
SOURCES += $$BIM_SRC/reg/registration.cpp

#---------------------------------------------------------------------
# libbioimage
#---------------------------------------------------------------------

DEFINES += BIM_USE_OPENMP
#DEFINES += BIM_USE_EIGEN
DEFINES += BIM_USE_TRANSFORMS
DEFINES += BIM_USE_FILTERS

INCLUDEPATH += $$DN_LIB_BIO
INCLUDEPATH += $$DN_FMTS_API
INCLUDEPATH += $$DN_FMTS
INCLUDEPATH += $$DN_CORE

PRE_TARGETDEPS = $$DN_LIB_BIO/.generated/libbioimage.a
LIBS += $$DN_LIB_BIO/.generated/libbioimage.a

BimLib.target = $$DN_LIB_BIO/.generated/libbioimage.a
#BimLib.commands = cd $$DN_LIB_BIO && qmake bioimage.pro && make
BimLib.depends = $$DN_LIB_BIO/Makefile
QMAKE_EXTRA_TARGETS += BimLib

#---------------------------------------------------------------------
# eigen
#---------------------------------------------------------------------

stat_eigen {
  INCLUDEPATH += $$DN_LIB_EIGEN
}

#---------------------------------------------------------------------
# jpeg-turbo
#---------------------------------------------------------------------

stat_libjpeg_turbo {

  win32 {
    LIBS += $$BIM_LIBS_PLTFM/turbojpeg-static.lib
  } else {
    #LIBS += $$BIM_LIBS_PLTFM/libjpeg.a
    LIBS += $$BIM_LIBS_PLTFM/libturbojpeg.a
  }

} # JPEG-TURBO

#---------------------------------------------------------------------
# LZMA
#---------------------------------------------------------------------

dyn_lzma {

  win32 {
    LIBS += $$BIM_LIBS_PLTFM/liblzma.lib
  } else:macx {
    #LIBS += $$BIM_LIBS_PLTFM/liblzma.a
  } else {
    LIBS += -llzma
  }

} # LZMA

#---------------------------------------------------------------------
# ffmpeg
#---------------------------------------------------------------------

ffmpeg {

  win32 {
    LIBS += $$BIM_LIBS_PLTFM/avcodec.lib
    LIBS += $$BIM_LIBS_PLTFM/avformat.lib
    LIBS += $$BIM_LIBS_PLTFM/avutil.lib
    LIBS += $$BIM_LIBS_PLTFM/swscale.lib
  } macx {
    LIBS += $$BIM_LIBS_PLTFM/libavformat.a
    LIBS += $$BIM_LIBS_PLTFM/libavcodec.a
    LIBS += $$BIM_LIBS_PLTFM/libswresample.a
    LIBS += $$BIM_LIBS_PLTFM/libswscale.a
    LIBS += $$BIM_LIBS_PLTFM/libavutil.a
    LIBS += $$BIM_LIBS_PLTFM/libvpx.a
    LIBS += $$BIM_LIBS_PLTFM/libx264.a
    LIBS += $$BIM_LIBS_PLTFM/libx265.a
    LIBS += $$BIM_LIBS_PLTFM/libxvidcore.a
    LIBS += $$BIM_LIBS_PLTFM/libogg.a
    LIBS += $$BIM_LIBS_PLTFM/libtheora.a
    LIBS += $$BIM_LIBS_PLTFM/libtheoraenc.a
    LIBS += $$BIM_LIBS_PLTFM/libtheoradec.a
    LIBS += -lpthread
    #LIBS += -framework CoreFoundation -framework VideoDecodeAcceleration -framework QuartzCore
  } else:unix {
    LIBS += $$BIM_LIBS_PLTFM/libavformat.a
    LIBS += $$BIM_LIBS_PLTFM/libavcodec.a
    LIBS += $$BIM_LIBS_PLTFM/libswresample.a
    LIBS += $$BIM_LIBS_PLTFM/libswscale.a
    LIBS += $$BIM_LIBS_PLTFM/libavutil.a
    LIBS += $$BIM_LIBS_PLTFM/libvpx.a
    LIBS += $$BIM_LIBS_PLTFM/libx264.a
    LIBS += $$BIM_LIBS_PLTFM/libx265.a
    LIBS += -lpthread -lxvidcore -lopenjpeg -lschroedinger-1.0 -ltheora -ltheoraenc -ltheoradec
  }

} # FFMPEG

#---------------------------------------------------------------------
# GDCM - under linux we only use system dynamic version right now
#---------------------------------------------------------------------

stat_gdcm {

  win32 {
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmjpeg12.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmjpeg16.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmjpeg8.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmcharls.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmCommon.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmDICT.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmDSED.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmexpat.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmgetopt.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmMEXD.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmMSFF.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmopenjpeg.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmzlib.lib
    LIBS += $$BIM_LIBS_PLTFM/gdcm/socketxx.lib
  } else {
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmDICT.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmMSFF.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmCommon.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmDSED.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmIOD.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmcharls.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmexpat.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmjpeg8.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmjpeg12.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmjpeg16.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmopenjpeg.a
    LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmzlib.a
  }

  macx {
    LIBS += -framework CoreFoundation
  }

} # static GDCM

dyn_gdcm {
  DEFINES += BIM_GDCM_FORMAT
  SOURCES += $$BIM_FMT_DICOM/bim_dicom_format.cpp

  win32 {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.lib
  } else:macx {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.a
  } else:unix {
    LIBS += -lgdcm
  }

} # System GDCM

#---------------------------------------------------------------------
# openjpeg
#---------------------------------------------------------------------

stat_openjpeg {
  unix {
    LIBS += $$BIM_LIBS_PLTFM/libopenjp2.a
  }
}

#---------------------------------------------------------------------
# jxrlib
#---------------------------------------------------------------------

stat_jxrlib {
  unix {
    LIBS += $$BIM_LIBS_PLTFM/libjxrglue.a
    LIBS += $$BIM_LIBS_PLTFM/libjpegxr.a
    LIBS += -lm
  }
}

#---------------------------------------------------------------------
# libwebp
#---------------------------------------------------------------------

stat_libwebp {
  unix {
    LIBS += $$BIM_LIBS_PLTFM/libwebp.a
    LIBS += $$BIM_LIBS_PLTFM/libwebpmux.a
    LIBS += $$BIM_LIBS_PLTFM/libwebpdemux.a
  }
}

#---------------------------------------------------------------------
# lcms2
#---------------------------------------------------------------------

stat_lcms2 {
  unix {
    LIBS += $$BIM_LIBS_PLTFM/liblcms2.a
  }
}

#---------------------------------------------------------------------
# required libs
#---------------------------------------------------------------------

unix:LIBS += -lbz2
unix:LIBS += -ldl

macx {
  LIBS += $$BIM_LIBS_PLTFM/libfftw3.a
  LIBS += -lz
} else:unix {
  LIBS += -lfftw3
}
