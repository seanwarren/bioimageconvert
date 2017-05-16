######################################################################
# Manually generated !!!
# BioImageConvert v 1.50 Project file
# run: qmake bimread.pro in order to generate Makefile for your platform
# Copyright (c) 2005-2010, Bio-Image Informatic Center, UCSB
#
# To generate Makefile on any platform:
#   qmake bimread.pro
#
# To generate VisualStudio project file:
#   qmake -t vcapp -spec win32-msvc2005 bimread.pro
#   qmake -t vcapp -spec win32-msvc.net bimread.pro
#   qmake -t vcapp -spec win32-msvc bimread.pro
#   qmake -spec win32-icc bimread.pro # to use pure Intel Compiler
#
# To generate xcode project file:
#   qmake -spec macx-xcode bimread.pro
#
# To generate Makefile on MacOSX with binary install:
#   qmake -spec macx-g++ bimread.pro
#
######################################################################

#---------------------------------------------------------------------
# configuration: editable
#---------------------------------------------------------------------
APP_NAME = bimread
TARGET = bimread

TEMPLATE = lib
CONFIG  += dll
VERSION = 2.0.0

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
CONFIG += ffmpeg
CONFIG += stat_exiv2
CONFIG += stat_eigen
CONFIG += libraw
CONFIG += sys_bzlib
CONFIG += stat_gdcm
#CONFIG += dyn_gdcm
CONFIG += stat_openjpeg
CONFIG += stat_jxrlib
CONFIG += stat_libwebp
CONFIG += stat_lcms2

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
  QMAKE_LFLAGS_DEBUG += -Wl,-Bsymbolic -pg -fPIC -ggdb

  QMAKE_CFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_CXXFLAGS_RELEASE += -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
  QMAKE_LFLAGS_RELEASE += -Wl,-Bsymbolic -fPIC -fopenmp -O3 -ftree-vectorize -msse2 -ffast-math -ftree-vectorizer-verbose=0
}

#---------------------------------------------------------------------
# configuration paths: editable
#---------------------------------------------------------------------

BIM_SRC = $${_PRO_FILE_PWD_}/../src_bimread
BIM_LSRC = $${_PRO_FILE_PWD_}/../libsrc
BIM_LIBS = $${_PRO_FILE_PWD_}/../libs
BIM_IMGS = $${_PRO_FILE_PWD_}/../images

HOSTTYPE = $$(HOSTTYPE)

unix {
  BIM_GENS = ../.generated/$$HOSTTYPE
  # path for object files
  BIM_OBJ = $$BIM_GENS/obj
  # path for generated binary
  BIM_BIN = ../$$HOSTTYPE
}
win32 {
  BIM_GENS = ../.generated/$(PlatformName)/$(ConfigurationName)
  # path for object files
  BIM_OBJ = $$BIM_GENS
  # path for generated binary
  BIM_BIN = ../$(PlatformName)/$(ConfigurationName)
}

BIM_LIB_TIF = $$BIM_LSRC/libtiff
stat_libjpeg_turbo {
  BIM_LIB_JPG    = $$BIM_LSRC/libjpeg-turbo
} else {
  BIM_LIB_JPG = $$BIM_LSRC/libjpeg
}
BIM_LIB_PNG = $$BIM_LSRC/libpng
BIM_LIB_Z   = $$BIM_LSRC/zlib
BIM_LIB_BZ2 = $$BIM_LSRC/bzip2
BIM_LIB_BIO = $$BIM_LSRC/libbioimg

BIM_CORE     = $$BIM_LIB_BIO/core_lib
BIM_FMTS     = $$BIM_LIB_BIO/formats
BIM_FMTS_API = $$BIM_LIB_BIO/formats_api

ffmpeg {
  BIM_LIB_FFMPEG = $$BIM_LSRC/ffmpeg
  BIM_FMT_FFMPEG = $$BIM_FMTS/mpeg
}
BIM_LIB_EXIV2   = $$BIM_LSRC/exiv2
BIM_LIB_EIGEN   = $$BIM_LSRC/eigen
BIM_LIB_RAW     = $$BIM_LSRC/libraw
BIM_LIB_GDCM   = $$BIM_LSRC/gdcm

#---------------------------------------------------------------------
# configuration: matlab
#---------------------------------------------------------------------

# Matlab path, required!!!
# Start by trying if there's MATLAB environment variable
MATLAB = $$(MATLAB)
MATLAB_TEST = $$MATLAB/extern/src/mexversion.c
!exists( $$MATLAB_TEST ) {
  #some default locations
  unix:MATLAB = /usr/local/matlab
  linux-g++-64:MATLAB = /usr/local/matlab
  win32:MATLAB = C:\MATLAB
    
  MATLAB_TEST = $$MATLAB/extern/src/mexversion.c
  !exists( $$MATLAB_TEST ) {
    message( "Fatal error: Cannot find Matlab on your machine!!!" )
    error( "Try setting MATLAB environment variable or alter bimread.pro with the right path" )
  }
}

##################################
# Matlab
# platform extensions: 
#      Windows x86     - .mexw32
#      Windows x64     - .mexw64
#      Linux 32bit     - .mexglx
#      Linux x86-64    - .mexa64
#      MacOS X PPC     - .mexmac
#      MacOS X Intel   - .mexmaci
#      solaris         - .mexsol
#      hpux            - .mexhpux
#
# platform paths:
#      Windows x86     - $$MATLAB/extern/lib/win32/microsoft
#      Windows x64     - $$MATLAB/extern/lib/win64/microsoft
#      Linux 32bit     - $$MATLAB/bin/glnx86
#      Linux x86-64    - $$MATLAB/bin/glnxa64
#      MacOS X PPC     - 
#      MacOS X Intel   - $$MATLAB/bin/maci/ (libmex.dylib)
##################################

unix:DEFINES += MATLAB_MEX_FILE

MATLAB_INC  = $$MATLAB/extern/include
MATLAB_SRC  = $$MATLAB/extern/src

# first generic platform rules
unix {
  MATLAB_LIB = $$MATLAB/bin/glnx86
  MATLAB_EXT = mexglx  
}

win32 {
  MATLAB_LIB = $$MATLAB/extern/lib/win32/microsoft
  MATLAB_EXT = mexw32  
}

win64 {
  MATLAB_LIB = $$MATLAB/extern/lib/win64/microsoft
  MATLAB_EXT = mexw64  
}

# PPC Mac platform
macx {
  MATLAB_LIB = $$MATLAB/bin/mac
  MATLAB_EXT = mexmac  
}

# Intel Mac platform (comment if running on ppc mac)
macx {
  MATLAB_LIB = $$MATLAB/bin/maci
  MATLAB_EXT = mexmaci  
}

# more specific platform rules

macx-icc {
  MATLAB_LIB = $$MATLAB/bin/maci
  MATLAB_EXT = mexmaci  
}

linux-g++-64 {
  MATLAB_LIB = $$MATLAB/bin/glnxa64
  MATLAB_EXT = mexa64    
}

linux-ecc-64 {
  MATLAB_LIB = $$MATLAB/bin/glnxa64
  MATLAB_EXT = mexa64    
}

win32-icc {
  MATLAB_LIB = $$MATLAB/extern/lib/win32/icc
  MATLAB_EXT = mexw32  
}

#win32-msvc {
#  MATLAB_LIB = $$MATLAB/extern/lib/win32/microsoft/msvc60
#  MATLAB_EXT = mexw32  
#}
#
#win32-msvc2005 {
#  MATLAB_LIB = $$MATLAB/extern/lib/win32/microsoft
#  MATLAB_EXT = mexw32  
#}

SOURCES += $$MATLAB_SRC/mexversion.c

QMAKE_TARGET = $$APP_NAME
QMAKE_EXTENSION_SHLIB = $$MATLAB_EXT
TARGET = $$APP_NAME 

# linking matlab stuff
!macx {
unix:LIBS += $$MATLAB_LIB/libut.so
unix:LIBS += $$MATLAB_LIB/libmx.so
unix:LIBS += $$MATLAB_LIB/libmex.so
}

win32:LIBS += $$MATLAB_LIB/libut.lib  
win32:LIBS += $$MATLAB_LIB/libmx.lib  
win32:LIBS += $$MATLAB_LIB/libmex.lib 

macx:LIBS += $$MATLAB_LIB/libut.dylib
macx:LIBS += $$MATLAB_LIB/libmx.dylib
macx:LIBS += $$MATLAB_LIB/libmex.dylib

#---------------------------------------------------------------------
# configuration: automatic
#---------------------------------------------------------------------

win32 {
  DEFINES += _CRT_SECURE_NO_WARNINGS
}

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
# generation: required
#---------------------------------------------------------------------

CONFIG  -= qt x11 windows

MOC_DIR = $$BIM_GENS
DESTDIR = $$BIM_BIN
OBJECTS_DIR = $$BIM_OBJ
INCLUDEPATH += $$BIM_GENS

#---------------------------------------------------------------------
# main sources
#---------------------------------------------------------------------

SOURCES += $$BIM_SRC/mex_bimread.cpp

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
INCLUDEPATH += $$BIM_FMTS/tiff
INCLUDEPATH += $$BIM_CORE

unix:LIBS += -lbz2
unix:LIBS += -ldl
#SUBDIRS = $$BIM_LIB_BIO/bioimage.pro

macx {
  LIBS += $$BIM_LIBS_PLTFM/libfftw3.a
  LIBS += -lz
} else:unix {
  LIBS += -lfftw3
}

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
