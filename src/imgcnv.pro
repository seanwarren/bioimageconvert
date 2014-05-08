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

TEMPLATE = app
VERSION = 0.1.65

CONFIG = console

CONFIG += release
#CONFIG += debug
CONFIG += warn_off

# static library config
CONFIG += stat_libtiff
CONFIG += stat_libjpeg
CONFIG += stat_libpng
CONFIG += stat_zlib
CONFIG += ffmpeg
CONFIG += sys_bzlib
CONFIG += stat_exiv2
CONFIG += stat_eigen
CONFIG += libraw

CONFIG(debug, debug|release) {
   message(Building in DEBUG mode!)
   DEFINES += _DEBUG _DEBUG_
}

unix {
  QMAKE_CFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_CXXFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_LFLAGS_DEBUG += -pg -fPIC -ggdb

  QMAKE_CFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
}

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5


#---------------------------------------------------------------------
# configuration paths: editable
#---------------------------------------------------------------------

DN_SRC  = ./
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

win32: {
  DN_LIBS_PLTFM = $$DN_LIBS/vc2008
} else:macx {
  DN_LIBS_PLTFM = $$DN_LIBS/macosx
} else:unix {
  DN_LIBS_PLTFM = $$DN_LIBS/linux/$$HOSTTYPE
} else {
  DN_LIBS_PLTFM = $$DN_LIBS/linux
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
# library configuration: automatic
#---------------------------------------------------------------------

# mac os x
macx {
  CONFIG -= stat_zlib
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

SOURCES += main.cpp
SOURCES += reg/registration.cpp

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

unix:LIBS += -lbz2
unix:LIBS += -ldl
#SUBDIRS = $$DN_LIB_BIO/bioimage.pro

unix {
  LIBS += -lfftw3
}
macx {
  LIBS -= -lfftw3
  LIBS += $$DN_LIBS_PLTFM/libfftw3.a
}

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
# ffmpeg
#---------------------------------------------------------------------

ffmpeg {

  unix:LIBS += -lpthread -lxvidcore -lopenjpeg -lschroedinger-1.0 -ltheora -ltheoraenc -ltheoradec

  win32 {   
    LIBS += $$DN_LIBS_PLTFM/avcodec.lib
    LIBS += $$DN_LIBS_PLTFM/avformat.lib
    LIBS += $$DN_LIBS_PLTFM/avutil.lib
    LIBS += $$DN_LIBS_PLTFM/swscale.lib
  } else {
    LIBS += $$DN_LIBS_PLTFM/libavformat.a
    LIBS += $$DN_LIBS_PLTFM/libavcodec.a
    LIBS += $$DN_LIBS_PLTFM/libswscale.a
    LIBS += $$DN_LIBS_PLTFM/libavutil.a
    LIBS += $$DN_LIBS_PLTFM/libvpx.a      
    LIBS += $$DN_LIBS_PLTFM/libx264.a      
  }
  macx {
    LIBS += $$DN_LIBS_PLTFM/libx264.a
    LIBS += $$DN_LIBS_PLTFM/libvpx.a 
    LIBS += $$DN_LIBS_PLTFM/libxvidcore.a
    LIBS += $$DN_LIBS_PLTFM/libogg.a 
    LIBS += $$DN_LIBS_PLTFM/libtheora.a 
    LIBS += $$DN_LIBS_PLTFM/libtheoraenc.a  
    LIBS += $$DN_LIBS_PLTFM/libtheoradec.a 
    LIBS += -framework CoreFoundation -framework VideoDecodeAcceleration -framework QuartzCore        
  }
  
} # FFMPEG 
          
