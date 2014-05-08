######################################################################
# Manually generated !!!
# bimread v 1.22 Project file
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

TEMPLATE = lib
CONFIG  += dll

VERSION = 0.1.22
 
CONFIG += console 
CONFIG += release
#CONFIG += debug
CONFIG += warn_off

# static library config
# nothing defined uses dynamic system version
# defining "stat_" - uses embedded version
# defining "stat_sys_" - uses static system version

CONFIG += stat_libtiff
#CONFIG += stat_sys_libtiff

CONFIG += stat_libjpeg
#CONFIG += stat_sys_libjpeg

CONFIG += stat_libpng
#CONFIG += stat_sys_libpng

CONFIG += stat_zlib
#CONFIG += stat_sys_zlib

# lib ffmpeg at this point is forced to be local copy since the trunc changes too much
CONFIG += ffmpeg
#CONFIG += sys_ffmpeg

#CONFIG += stat_bzlib
CONFIG += sys_bzlib
macx:CONFIG += sys_bzlib

CONFIG += stat_exiv2
#CONFIG += sys_exiv2

CONFIG += stat_eigen

debug: {
   message(Building in DEBUG mode!)
   DEFINES += DEBUG _DEBUG _DEBUG_
}

unix {
  QMAKE_CFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_CXXFLAGS_DEBUG += -pg -fPIC -ggdb
  QMAKE_LFLAGS_DEBUG += -pg -fPIC -ggdb

  QMAKE_CFLAGS_RELEASE += -fPIC
  QMAKE_CXXFLAGS_RELEASE += -fPIC
  QMAKE_LFLAGS_RELEASE += -fPIC
}

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4

# Matlab path, required!!!
# Start by trying if there's MATLAB environment variable
MATLAB = $$(MATLAB)
MATLAB_TEST = $$MATLAB/extern/src/mexversion.c
!exists( $$MATLAB_TEST ) {

  #some default locations
  unix:MATLAB = /usr/local/matlab
  linux-g++-64:MATLAB = /usr/local/matlab
  win32:MATLAB = C:\MATLAB7
    
  MATLAB_TEST = $$MATLAB/extern/src/mexversion.c
  !exists( $$MATLAB_TEST ) {
    message( "Fatal error: Cannot find Matlab on your machine!!!" )
    error( "Try setting MATLAB environment variable or alter bimread.pro with the right path" )
  }
}

#---------------------------------------------------------------------
# configuration paths: editable
#---------------------------------------------------------------------

DN_SRC  = ./
DN_LSRC = ../libsrc
DN_LIBS = ../libs
DN_IMGS = ../images
unix {
  DN_GENS = ../generated/$$(HOSTTYPE)
  # path for object files
  DN_OBJ = $$DN_GENS/obj
  # path for generated binary
  DN_BIN = ../$$(HOSTTYPE)
}
win32 {
  DN_GENS = ../generated/$(PlatformName)/$(ConfigurationName)
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
  DN_LIBS_PLTFM = $$DN_LIBS/vc2008
} else:macx {
  DN_LIBS_PLTFM = $$DN_LIBS/macosx
} else:unix {
  DN_LIBS_PLTFM = $$DN_LIBS/linux/$$(HOSTTYPE)
} else {
  DN_LIBS_PLTFM = $$DN_LIBS/linux
}

unix {
  !exists( $$DN_GENS ) {
    message( "Cannot find directory: $$DN_GENS, creating..." )
    system( mkdir -p $$DN_GENS )
  }
}
win32 {
  !exists( $$DN_GENS ) {
    message( "Cannot find directory: $$DN_GENS, creating..." )
    !exists( ../generated/Win32/Debug       ) { system( mkdir ..\\generated\\Win32\\Debug ) }
    !exists( ../generated/Win32/Release     ) { system( mkdir ..\\generated\\Win32\\Release ) }
    !exists( ../generated/Win32/ICC_Release ) { system( mkdir ..\\generated\\Win32\\ICC_Release ) }
    !exists( ../generated/x64/Debug         ) { system( mkdir ..\\generated\\x64\\Debug ) }
    !exists( ../generated/x64/Release       ) { system( mkdir ..\\generated\\x64\\Release ) }
    !exists( ../generated/x64/ICC_Release   ) { system( mkdir ..\\generated\\x64\\ICC_Release ) }
  }
}

unix {
  !exists( $$DN_OBJ ) {
    message( "Cannot find directory: $$DN_OBJ, creating..." )
    system( mkdir -p $$DN_OBJ )
  }
}
win32 {
  !exists( $$DN_OBJ ) {
    message( "Cannot find directory: $$DN_GENS, creating..." )
    !exists( ../generated/Win32/Debug   ) { system( mkdir ..\\generated\\Win32\\Debug ) }
    !exists( ../generated/Win32/Release ) { system( mkdir ..\\generated\\Win32\\Release ) }
    !exists( ../generated/x64/Debug     ) { system( mkdir ..\\generated\\x64\\Debug ) }
    !exists( ../generated/x64/Release   ) { system( mkdir ..\\generated\\x64\\Release ) }
  }
}

# compile ffmpeg
ffmpeg {
  #message( "Compiling FFMPEG as requested..." )
  #win32 {
  #  message( "Attention: Cygwin and VC++2005 are required for ffmpeg windows compilation" )  
  #  system( cd $$DN_LIB_FFMPEG ; sh ./build-ffmpeg-cygwin.sh ; cd ../../ )
  #} else:macx {
  #  system( cd $$DN_LIB_FFMPEG ; sh ./build-ffmpeg-macosx.sh ; cd ../../ )
  #} else {
  #  system( cd $$DN_LIB_FFMPEG ; sh ./build-ffmpeg-linux.sh ; cd ../../ )  
  #}  
}

#---------------------------------------------------------------------
# library configuration: automatic
#---------------------------------------------------------------------

# By default all libraries are static, we can check for existing
# ones and use dynamic linking for those

unix {

  exists( $$DN_GENS ) {
    #checking for libtiff... -ltiff
    #CONFIG -= stat_libtiff
  }
  
  exists( $$DN_GENS ) {
    #checking for libjpeg... -ljpeg
    #CONFIG -= stat_libjpeg
  }
  
  exists( $$DN_GENS ) {  
    #checking for libpng... -lpng -lz -lm
    #CONFIG -= stat_libpng
  }

  exists( $$DN_GENS ) {
    #checking for libz... -lz
    #CONFIG -= stat_zlib
  }

}

# mac os x
macx {
  CONFIG -= stat_zlib
}


#---------------------------------------------------------------------
# generation: fixed
#---------------------------------------------------------------------

CONFIG  -= qt x11 windows

MOC_DIR = $$DN_GENS
DESTDIR = $$DN_BIN
OBJECTS_DIR = $$DN_OBJ

INCLUDEPATH += $$DN_GENS
DEPENDPATH += $$DN_GENS

INCLUDEPATH += $$MATLAB_INC

#---------------------------------------------------------------------
# main sources
#---------------------------------------------------------------------

#Main           
SOURCES += mex_bimread.cpp

#---------------------------------------------------------------------
#image formats library           
#---------------------------------------------------------------------

INCLUDEPATH += $$DN_LIB_BIO
INCLUDEPATH += $$DN_FMTS_API
INCLUDEPATH += $$DN_FMTS
INCLUDEPATH += $$DN_CORE

#core
SOURCES += $$DN_CORE/xstring.cpp $$DN_CORE/xtypes.cpp \
           $$DN_CORE/tag_map.cpp $$DN_CORE/xpointer.cpp $$DN_CORE/xconf.cpp

#Formats API 
SOURCES += $$DN_FMTS_API/dim_img_format_utils.cpp \
           $$DN_FMTS_API/dim_buffer.cpp \
           $$DN_FMTS_API/dim_histogram.cpp \
           $$DN_FMTS_API/bim_metatags.cpp \
           $$DN_FMTS_API/dim_image.cpp \
           $$DN_FMTS_API/dim_image_pyramid.cpp \
           $$DN_FMTS_API/dim_image_stack.cpp
           
#Formats     
SOURCES += $$DN_FMTS/dim_format_manager.cpp \
           $$DN_FMTS/meta_format_manager.cpp\
           $$DN_FMTS/tiff/dim_tiff_format.cpp \
           $$DN_FMTS/tiff/dim_xtiff.c \
           $$DN_FMTS/tiff/memio.c \
           $$DN_FMTS/dmemio.cpp \
           $$DN_FMTS/jpeg/dim_jpeg_format.cpp \
           $$DN_FMTS/biorad_pic/dim_biorad_pic_format.cpp \
           $$DN_FMTS/bmp/dim_bmp_format.cpp \
           $$DN_FMTS/png/dim_png_format.cpp \
           $$DN_FMTS/nanoscope/dim_nanoscope_format.cpp \
           $$DN_FMTS/raw/dim_raw_format.cpp \
           $$DN_FMTS/ibw/dim_ibw_format.cpp \
           $$DN_FMTS/ome/dim_ome_format.cpp\
           $$DN_FMTS/ole/dim_ole_format.cpp\
           $$DN_FMTS/ole/zvi.cpp

#---------------------------------------------------------------------        
#Pole

D_LIB_POLE = $$DN_LSRC/pole
INCLUDEPATH += $$D_LIB_POLE

SOURCES += $$D_LIB_POLE/pole.cpp

#---------------------------------------------------------------------        
#ffmpeg

ffmpeg {

  DEFINES  += DIM_FFMPEG_FORMAT FFMPEG_VIDEO_DISABLE_MATLAB
  INCLUDEPATH += $$DN_LIB_FFMPEG/include
  #INCLUDEPATH += $$DN_LIB_FFMPEG/ffmpeg-out/$$(HOSTTYPE)/include
  win32:INCLUDEPATH += $$DN_LIB_FFMPEG/include-win

  SOURCES += $$DN_FMT_FFMPEG/debug.cpp $$DN_FMT_FFMPEG/dim_ffmpeg_format.cpp \
             $$DN_FMT_FFMPEG/FfmpegCommon.cpp $$DN_FMT_FFMPEG/FfmpegIVideo.cpp \
             $$DN_FMT_FFMPEG/FfmpegOVideo.cpp $$DN_FMT_FFMPEG/registry.cpp
	   
  win32 {   
    LIBS += $$DN_LIBS_PLTFM/avcodec-51.lib
    LIBS += $$DN_LIBS_PLTFM/avformat-52.lib
    LIBS += $$DN_LIBS_PLTFM/avutil-50.lib
    LIBS += $$DN_LIBS_PLTFM/swscale-0.lib
  } else {
    LIBS += $$DN_LIBS_PLTFM/libavformat.a
    LIBS += $$DN_LIBS_PLTFM/libavcodec.a
    LIBS += $$DN_LIBS_PLTFM/libswscale.a
    LIBS += $$DN_LIBS_PLTFM/libavutil.a  
  }
} # FFMPEG            

 
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
#LibTiff

stat_libtiff {
  INCLUDEPATH += $$DN_LIB_TIF
  SOURCES += $$DN_LIB_TIF/tif_fax3sm.c $$DN_LIB_TIF/tif_aux.c \
             $$DN_LIB_TIF/tif_close.c $$DN_LIB_TIF/tif_codec.c \
             $$DN_LIB_TIF/tif_color.c $$DN_LIB_TIF/tif_compress.c \
             $$DN_LIB_TIF/tif_dir.c $$DN_LIB_TIF/tif_dirinfo.c \
             $$DN_LIB_TIF/tif_dirread.c $$DN_LIB_TIF/tif_dirwrite.c \
             $$DN_LIB_TIF/tif_dumpmode.c $$DN_LIB_TIF/tif_error.c \
             $$DN_LIB_TIF/tif_extension.c $$DN_LIB_TIF/tif_fax3.c \
             $$DN_LIB_TIF/tif_flush.c $$DN_LIB_TIF/tif_getimage.c \
             $$DN_LIB_TIF/tif_jpeg.c $$DN_LIB_TIF/tif_luv.c \
             $$DN_LIB_TIF/tif_lzw.c $$DN_LIB_TIF/tif_next.c \
             $$DN_LIB_TIF/tif_open.c $$DN_LIB_TIF/tif_packbits.c \
             $$DN_LIB_TIF/tif_pixarlog.c $$DN_LIB_TIF/tif_predict.c \
             $$DN_LIB_TIF/tif_print.c $$DN_LIB_TIF/tif_read.c \
             $$DN_LIB_TIF/tif_strip.c $$DN_LIB_TIF/tif_swab.c \
             $$DN_LIB_TIF/tif_thunder.c $$DN_LIB_TIF/tif_tile.c \
             $$DN_LIB_TIF/tif_version.c $$DN_LIB_TIF/tif_warning.c \
             $$DN_LIB_TIF/tif_write.c $$DN_LIB_TIF/tif_zip.c
           
  unix:SOURCES  += $$DN_LIB_TIF/tif_unix.c
  win32:SOURCES += $$DN_LIB_TIF/tif_win32.c
}

!stat_libtiff {
  unix {
    stat_sys_libtiff {
      LIBS += /usr/lib/libtiff.a
    } else {
      LIBS += -ltiff
    }
  }
  win32:LIBS += $$DN_LIBS_PLTFM/libtiff.lib
}

#---------------------------------------------------------------------        
#LibPng

stat_libpng {

  INCLUDEPATH += $$DN_LIB_PNG
  # by default disable intel asm code
  unix:DEFINES += PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD

  # enable only for x86 machines (not x64)
  #message( "Enable Intel ASM code for PNG..." )
  win32:DEFINES -= PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD
  macx:DEFINES -= PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD
  linux-g++-32:DEFINES -= PNG_NO_ASSEMBLER_CODE PNG_USE_PNGVCRD

  SOURCES += $$DN_LIB_PNG/png.c $$DN_LIB_PNG/pngerror.c $$DN_LIB_PNG/pngget.c \
             $$DN_LIB_PNG/pngmem.c $$DN_LIB_PNG/pngpread.c $$DN_LIB_PNG/pngread.c \
             $$DN_LIB_PNG/pngrio.c $$DN_LIB_PNG/pngrtran.c $$DN_LIB_PNG/pngrutil.c \
             $$DN_LIB_PNG/pngset.c $$DN_LIB_PNG/pngtrans.c \
             $$DN_LIB_PNG/pngwio.c $$DN_LIB_PNG/pngwrite.c $$DN_LIB_PNG/pngwtran.c \
             $$DN_LIB_PNG/pngwutil.c
}

!stat_libpng {
  unix {
    stat_sys_libpng {
      LIBS += /usr/lib/libpng.a
    } else {
      LIBS += -lpng
    }
  }
  win32:LIBS += $$DN_LIBS_PLTFM/libpng.lib
}

  
#---------------------------------------------------------------------         
#ZLib

stat_zlib {
  INCLUDEPATH += $$DN_LIB_Z
  SOURCES += $$DN_LIB_Z/adler32.c $$DN_LIB_Z/compress.c $$DN_LIB_Z/crc32.c \
             $$DN_LIB_Z/deflate.c $$DN_LIB_Z/gzio.c $$DN_LIB_Z/infback.c \
             $$DN_LIB_Z/inffast.c $$DN_LIB_Z/inflate.c $$DN_LIB_Z/inftrees.c \
             $$DN_LIB_Z/trees.c $$DN_LIB_Z/uncompr.c $$DN_LIB_Z/zutil.c
}

!stat_zlib {
  unix {
    stat_sys_zlib {
      LIBS += /usr/lib/libz.a
    } else {
      LIBS += -lz
    }
  }
  win32:LIBS += $$DN_LIBS_PLTFM/zlib.lib  
}

#---------------------------------------------------------------------
#libjpeg

stat_libjpeg {
  INCLUDEPATH += $$DN_LIB_JPG
  SOURCES += $$DN_LIB_JPG/jaricom.c $$DN_LIB_JPG/jcapimin.c $$DN_LIB_JPG/jcapistd.c \
       $$DN_LIB_JPG/jcarith.c $$DN_LIB_JPG/jccoefct.c \
	     $$DN_LIB_JPG/jccolor.c $$DN_LIB_JPG/jcdctmgr.c $$DN_LIB_JPG/jchuff.c \
	     $$DN_LIB_JPG/jcinit.c $$DN_LIB_JPG/jcmainct.c $$DN_LIB_JPG/jcmarker.c \
	     $$DN_LIB_JPG/jcmaster.c $$DN_LIB_JPG/jcomapi.c $$DN_LIB_JPG/jcparam.c \
	     $$DN_LIB_JPG/jcprepct.c $$DN_LIB_JPG/jcsample.c $$DN_LIB_JPG/jctrans.c \
	     $$DN_LIB_JPG/jdapimin.c $$DN_LIB_JPG/jdapistd.c $$DN_LIB_JPG/jdarith.c \
	     $$DN_LIB_JPG/jdatadst.c $$DN_LIB_JPG/jdatasrc.c $$DN_LIB_JPG/jdcoefct.c \
	     $$DN_LIB_JPG/jdcolor.c $$DN_LIB_JPG/jddctmgr.c $$DN_LIB_JPG/jdhuff.c \
	     $$DN_LIB_JPG/jdinput.c $$DN_LIB_JPG/jdmainct.c $$DN_LIB_JPG/jdmarker.c \
	     $$DN_LIB_JPG/jdmaster.c $$DN_LIB_JPG/jdmerge.c \
	     $$DN_LIB_JPG/jdpostct.c $$DN_LIB_JPG/jdsample.c $$DN_LIB_JPG/jdtrans.c \
	     $$DN_LIB_JPG/jerror.c $$DN_LIB_JPG/jfdctflt.c $$DN_LIB_JPG/jfdctfst.c \
	     $$DN_LIB_JPG/jfdctint.c $$DN_LIB_JPG/jidctflt.c $$DN_LIB_JPG/jidctfst.c \
	     $$DN_LIB_JPG/jidctint.c $$DN_LIB_JPG/jmemmgr.c \
	     $$DN_LIB_JPG/jquant1.c $$DN_LIB_JPG/jquant2.c $$DN_LIB_JPG/jutils.c \
	     $$DN_LIB_JPG/jmemansi.c
}

!stat_libjpeg {
  unix {
    stat_sys_libjpeg {
      LIBS += /usr/lib/libjpeg.a
    } else {
      LIBS += -ljpeg
    }
  }
  win32:LIBS += $$DN_LIBS_PLTFM/libjpeg.lib
}   

#---------------------------------------------------------------------
#bzlib
   
stat_bzlib {
  INCLUDEPATH += $$DN_LIB_BZ2
  SOURCES += $$DN_LIB_BZ2/blocksort.c $$DN_LIB_BZ2/bzlib.c $$DN_LIB_BZ2/randtable.c \
             $$DN_LIB_BZ2/compress.c $$DN_LIB_BZ2/crctable.c $$DN_LIB_BZ2/decompress.c $$DN_LIB_BZ2/huffman.c
} 

sys_bzlib {
   unix:LIBS += -lbz2
   macx:LIBS += -lbz2     
   #win32:LIBS += $$DN_LIBS_PLTFM/libbz2.lib     
}
  
#---------------------------------------------------------------------
#exiv2

stat_exiv2 {
  INCLUDEPATH += $$DN_LIB_EXIV2
  INCLUDEPATH += $$DN_LIB_EXIV2/exiv2  
  SOURCES += $$DN_LIB_EXIV2/exiv2/basicio.cpp $$DN_LIB_EXIV2/exiv2/bmpimage.cpp \
	  $$DN_LIB_EXIV2/exiv2/canonmn.cpp $$DN_LIB_EXIV2/exiv2/convert.cpp $$DN_LIB_EXIV2/exiv2/cr2image.cpp \
	  $$DN_LIB_EXIV2/exiv2/crwimage.cpp $$DN_LIB_EXIV2/exiv2/datasets.cpp $$DN_LIB_EXIV2/exiv2/easyaccess.cpp \
	  $$DN_LIB_EXIV2/exiv2/error.cpp $$DN_LIB_EXIV2/exiv2/exif.cpp $$DN_LIB_EXIV2/exiv2/futils.cpp \
	  $$DN_LIB_EXIV2/exiv2/fujimn.cpp $$DN_LIB_EXIV2/exiv2/gifimage.cpp $$DN_LIB_EXIV2/exiv2/image.cpp \
	  $$DN_LIB_EXIV2/exiv2/iptc.cpp $$DN_LIB_EXIV2/exiv2/jp2image.cpp $$DN_LIB_EXIV2/exiv2/jpgimage.cpp \
	  $$DN_LIB_EXIV2/exiv2/makernote.cpp $$DN_LIB_EXIV2/exiv2/metadatum.cpp $$DN_LIB_EXIV2/exiv2/minoltamn.cpp \
	  $$DN_LIB_EXIV2/exiv2/mrwimage.cpp $$DN_LIB_EXIV2/exiv2/nikonmn.cpp $$DN_LIB_EXIV2/exiv2/olympusmn.cpp \
	  $$DN_LIB_EXIV2/exiv2/orfimage.cpp $$DN_LIB_EXIV2/exiv2/panasonicmn.cpp $$DN_LIB_EXIV2/exiv2/pgfimage.cpp \
    $$DN_LIB_EXIV2/exiv2/pngimage.cpp $$DN_LIB_EXIV2/exiv2/pngchunk.cpp $$DN_LIB_EXIV2/exiv2/preview.cpp \
	  $$DN_LIB_EXIV2/exiv2/properties.cpp $$DN_LIB_EXIV2/exiv2/psdimage.cpp $$DN_LIB_EXIV2/exiv2/rafimage.cpp \
	  $$DN_LIB_EXIV2/exiv2/rw2image.cpp $$DN_LIB_EXIV2/exiv2/sigmamn.cpp $$DN_LIB_EXIV2/exiv2/pentaxmn.cpp \
	  $$DN_LIB_EXIV2/exiv2/sonymn.cpp $$DN_LIB_EXIV2/exiv2/tags.cpp $$DN_LIB_EXIV2/exiv2/tgaimage.cpp \
	  $$DN_LIB_EXIV2/exiv2/tiffcomposite.cpp $$DN_LIB_EXIV2/exiv2/tiffimage.cpp $$DN_LIB_EXIV2/exiv2/tiffvisitor.cpp \
	  $$DN_LIB_EXIV2/exiv2/types.cpp $$DN_LIB_EXIV2/exiv2/value.cpp $$DN_LIB_EXIV2/exiv2/version.cpp \
	  $$DN_LIB_EXIV2/exiv2/xmp.cpp $$DN_LIB_EXIV2/exiv2/xmpsidecar.cpp
}

sys_exiv2 {
  unix:LIBS += -lexiv2
  win32:LIBS += $$DN_LIBS_PLTFM/libexiv2.lib
}  

#---------------------------------------------------------------------
#eigen

stat_eigen {
  INCLUDEPATH += $$DN_LIB_EIGEN
}

#---------------------------------------------------------------------

DEPENDPATH += $$INCLUDEPATH

#---------------------------------------------------------------------
#install target
staticlib {
  INSTALLHEADERS.files += $$DN_FMTS/dim_format_manager.h \
                          $$DN_FMTS/meta_format_manager.h \
                          $$DN_FMTS_API/dim_img_format_interface.h \
                          $$DN_FMTS_API/dim_img_format_utils.h \
                          $$DN_FMTS_API/dim_image.h \
                          $$DN_FMTS_API/dim_histogram.h \
                          $$DN_FMTS_API/dim_buffer.h \
                          $$DN_CORE/tag_map.h
  INSTALLHEADERS.files += $$DN_LIBS_PLTFM/adler32.h \
                          $$DN_LIBS_PLTFM/avcodec.h $$DN_LIBS_PLTFM/avdevice.h \
                          $$DN_LIBS_PLTFM/avformat.h $$DN_LIBS_PLTFM/avio.h \
                          $$DN_LIBS_PLTFM/avstring.h $$DN_LIBS_PLTFM/avutil.h \
                          $$DN_LIBS_PLTFM/base64.h $$DN_LIBS_PLTFM/common.h \
                          $$DN_LIBS_PLTFM/crc.h $$DN_LIBS_PLTFM/fifo.h \
                          $$DN_LIBS_PLTFM/intfloat_readwrite.h $$DN_LIBS_PLTFM/log.h \
                          $$DN_LIBS_PLTFM/lzo.h $$DN_LIBS_PLTFM/mathematics.h \
                          $$DN_LIBS_PLTFM/md5.h $$DN_LIBS_PLTFM/mem.h \
                          $$DN_LIBS_PLTFM/opt.h $$DN_LIBS_PLTFM/random.h \
                          $$DN_LIBS_PLTFM/rational.h $$DN_LIBS_PLTFM/rgb2rgb.h \
                          $$DN_LIBS_PLTFM/rtspcodes.h $$DN_LIBS_PLTFM/rtsp.h \
                          $$DN_LIBS_PLTFM/sha1.h $$DN_LIBS_PLTFM/swscale.h
  INSTALLHEADERS.path = /include
  INSTALLS += INSTALLHEADERS

  INSTALLBINS.files += $$DN_BIN/lib$${TARGET}.a \
                       $$DN_LIBS_PLTFM/libavcodec.a \
                       $$DN_LIBS_PLTFM/libavformat.a \
                       $$DN_LIBS_PLTFM/libavutil.a \
                       $$DN_LIBS_PLTFM/libswscale.a
  INSTALLBINS.path = /lib
} else {
  INSTALLBINS.files += $$DN_BIN/$${TARGET}
  INSTALLBINS.path = /bin
}
INSTALLS += INSTALLBINS
