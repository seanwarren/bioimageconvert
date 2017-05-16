######################################################################
# Manually generated !!!
# libBioImage v 1.55 Project file
# run:
#   qmake -r imgcnv.pro - in order to generate Makefile for your platform
#   make all - to compile the library
#
#
# Copyright (c) 2005-2010, Bio-Image Informatic Center, UCSB
#
# To generate Makefiles on any platform:
#   qmake imgcnv.pro
#
# To generate VisualStudio project files:
#   qmake -t vcapp -spec win32-msvc2005 imgcnv.pro
#   qmake -t vcapp -spec win32-msvc.net imgcnv.pro
#   qmake -t vcapp -spec win32-msvc imgcnv.pro
#   qmake -spec win32-icc imgcnv.pro # to use pure Intel Compiler
#
# To generate xcode project files:
#   qmake -spec macx-xcode imgcnv.pro
#
# To generate Makefiles on MacOSX with binary install:
#   qmake -spec macx-g++ imgcnv.pro
#
######################################################################

win32 {
    *-g++* {
        message(detected platform Windows with compiler gcc (typically MinGW, MinGW64, Cygwin, MSYS2, or similar))
        CONFIG += mingw
        GCCMACHINETYPE=$$system("gcc -dumpmachine")
        contains(GCCMACHINETYPE, x86_64.*):CONFIG += win64
    }
}

#---------------------------------------------------------------------
# configuration: editable
#---------------------------------------------------------------------
APP_NAME = imgcnv
TARGET = $$APP_NAME

TEMPLATE = app
VERSION = 2.1.1

CONFIG += console

CONFIG += release
#CONFIG += debug
CONFIG += warn_off

# static library config
CONFIG += stat_libtiff
#CONFIG += stat_libjpeg # pick one or the other
CONFIG += stat_libjpeg_turbo # pick one or the other
CONFIG += stat_libpng
CONFIG += stat_zlib
#CONFIG += ffmpeg
#CONFIG += stat_bzlib
CONFIG += dyn_bzlib
#CONFIG += stat_exiv2
CONFIG += dyn_exiv2
CONFIG += stat_eigen
CONFIG += stat_libraw
CONFIG += stat_openjpeg
#CONFIG += stat_jxrlib
CONFIG += dyn_jxrlib
#CONFIG += stat_libwebp
CONFIG += dyn_libwebp
CONFIG += stat_lcms2
CONFIG += dyn_lzma

CONFIG += stat_gdcm
#CONFIG += dyn_gdcm # ubuntu 16 comes with a reasonably new version of GDCM

macx {
    CONFIG += ffmpeg
} else:unix {
    CONFIG += dyn_ffmpeg # debian 8 and ubuntu 16 come with a reasonably new version of FFmpeg
}

CONFIG(debug, debug|release) {
  message(Building in DEBUG mode!)
  DEFINES += DEBUG _DEBUG _DEBUG_
}

macx {
  QMAKE_CFLAGS_RELEASE += -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE += -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE += -m64 -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
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

BIM_SRC  = $${_PRO_FILE_PWD_}/../src
BIM_LSRC = $${_PRO_FILE_PWD_}/../libsrc
BIM_LIBS = $${_PRO_FILE_PWD_}/../libs
BIM_IMGS = $${_PRO_FILE_PWD_}/../images

HOSTTYPE = $$(HOSTTYPE)

unix | mingw {
  BIM_GENS = ../.generated/$$HOSTTYPE
  # path for object files
  BIM_OBJ = $$BIM_GENS/obj
  # path for generated binary
  BIM_BIN = ../$$HOSTTYPE
} else:win32 {
  BIM_GENS = ../.generated/$(PlatformName)/$(ConfigurationName)
  # path for object files
  BIM_OBJ = $$BIM_GENS
  # path for generated binary
  BIM_BIN = ../$(PlatformName)/$(ConfigurationName)
}

DN_LIB_JPG = $$DN_LSRC/libjpeg
DN_LIB_PNG = $$DN_LSRC/libpng
DN_LIB_Z   = $$DN_LSRC/zlib
DN_LIB_BZ2 = $$DN_LSRC/bzip2
DN_LIB_BIO = $$DN_LSRC/libbioimg

BIM_LIB_TIF = $$BIM_LSRC/libtiff
stat_libjpeg_turbo {
  BIM_LIB_JPG    = $$BIM_LSRC/libjpeg-turbo
} else {
  BIM_LIB_JPG    = $$BIM_LSRC/libjpeg
}
BIM_LIB_PNG = $$BIM_LSRC/libpng
BIM_LIB_Z   = $$BIM_LSRC/zlib
BIM_LIB_BZ2 = $$BIM_LSRC/bzip2
BIM_LIB_BIO = $$BIM_LSRC/libbioimg

BIM_CORE     = $$BIM_LIB_BIO/core_lib
BIM_FMTS     = $$BIM_LIB_BIO/formats
BIM_FMTS_API = $$BIM_LIB_BIO/formats_api
BIM_TRANSFORMS   = $$BIM_LIB_BIO/transforms

ffmpeg {
  BIM_LIB_FFMPEG = $$BIM_LSRC/ffmpeg
  BIM_FMT_FFMPEG = $$BIM_FMTS/mpeg
}
BIM_LIB_EXIV2    = $$BIM_LSRC/exiv2
BIM_LIB_EIGEN    = $$BIM_LSRC/eigen
BIM_LIB_RAW      = $$BIM_LSRC/libraw
BIM_LIB_GDCM     = $$BIM_LSRC/gdcm

#---------------------------------------------------------------------
# configuration: automatic
#---------------------------------------------------------------------

win32:!mingw {
  DEFINES += _CRT_SECURE_NO_WARNINGS
}

BIM_LIBS_PLTFM = $$BIM_LIBS
mingw {
  BIM_LIBS_PLTFM = $$BIM_LIBS/mingw
} else:win32 {
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

win32 {
  !exists( $$BIM_GENS ) {
    message( "Cannot find directory: $$BIM_GENS, creating..." )
    !exists( ../.generated/Win32/Debug       ) { system( mkdir ..\\.generated\\Win32\\Debug ) }
    !exists( ../.generated/Win32/Release     ) { system( mkdir ..\\.generated\\Win32\\Release ) }
    !exists( ../.generated/Win32/ICC_Release ) { system( mkdir ..\\.generated\\Win32\\ICC_Release ) }
    !exists( ../.generated/x64/Debug         ) { system( mkdir ..\\.generated\\x64\\Debug ) }
    !exists( ../.generated/x64/Release       ) { system( mkdir ..\\.generated\\x64\\Release ) }
    !exists( ../.generated/x64/ICC_Release   ) { system( mkdir ..\\.generated\\x64\\ICC_Release ) }
  }
  !exists( $$BIM_OBJ ) {
    message( "Cannot find directory: $$BIM_GENS, creating..." )
    !exists( ../.generated/Win32/Debug   ) { system( mkdir ..\\.generated\\Win32\\Debug ) }
    !exists( ../.generated/Win32/Release ) { system( mkdir ..\\.generated\\Win32\\Release ) }
    !exists( ../.generated/x64/Debug     ) { system( mkdir ..\\.generated\\x64\\Debug ) }
    !exists( ../.generated/x64/Release   ) { system( mkdir ..\\.generated\\x64\\Release ) }
  }
}

#---------------------------------------------------------------------
# generation: fixed
#---------------------------------------------------------------------

CONFIG -= qt x11 windows

MOC_DIR = $$BIM_GENS
DESTDIR = $$BIM_BIN
OBJECTS_DIR = $$BIM_OBJ
INCLUDEPATH += $$BIM_GENS

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

INCLUDEPATH += $$BIM_LIB_BIO
INCLUDEPATH += $$BIM_FMTS_API
INCLUDEPATH += $$BIM_FMTS
INCLUDEPATH += $$BIM_CORE

PRE_TARGETDEPS = $$BIM_LIB_BIO/.generated/libbioimage.a
LIBS += $$BIM_LIB_BIO/.generated/libbioimage.a

BimLib.target = $$BIM_LIB_BIO/.generated/libbioimage.a
#BimLib.commands = cd $$BIM_LIB_BIO && qmake bioimage.pro && make
BimLib.depends = $$BIM_LIB_BIO/Makefile
QMAKE_EXTRA_TARGETS += BimLib

#---------------------------------------------------------------------
# eigen
#---------------------------------------------------------------------

stat_eigen {
  INCLUDEPATH += $$BIM_LIB_EIGEN
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
    LIBS += -llzma
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
    #LIBS += -lavformat
  } else:macx {
    #LIBS += -lavformat
  } else:unix {
    LIBS += -lavformat
    LIBS += -lavcodec
    LIBS += -lavutil
    LIBS += -lswresample
    LIBS += -lswscale
  }

} # System FFMPEG

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
  } else:macx {
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
    #LIBS += $$BIM_LIBS_PLTFM/gdcm/libgdcmzlib.a
    LIBS += -framework CoreFoundation
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

} # static GDCM

dyn_gdcm {
  win32 {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.lib
  } else:macx {
#    LIBS += $$BIM_LIBS_PLTFM/gdcm/gdcmIOD.a
  } else:unix {
    LIBS += -lgdcmDICT
    LIBS += -lgdcmMSFF
    LIBS += -lgdcmCommon
    LIBS += -lgdcmDSED
    LIBS += -lgdcmIOD
    LIBS += -lgdcmjpeg8
    LIBS += -lgdcmjpeg12
    LIBS += -lgdcmjpeg16
  }

} # System GDCM

#---------------------------------------------------------------------
# required libs
#---------------------------------------------------------------------

unix:LIBS += -lbz2
unix:LIBS += -ldl

macx {
  LIBS += $$BIM_LIBS_PLTFM/libfftw3.a
  LIBS += -liconv
  LIBS += -lz
} else:unix {
  LIBS += -lfftw3
}
