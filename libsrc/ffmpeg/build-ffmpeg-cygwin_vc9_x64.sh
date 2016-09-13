#!/bin/bash

chmod a+x ./ffmpeg/configure
chmod a+x ./ffmpeg/version.sh
chmod a+x ./ffmpeg/doc/texi2pod.pl

rm -Rf ./ffmpeg-obj
rm -Rf ./ffmpeg-out

mkdir -p ffmpeg-out
#cd ffmpeg-obj
cd ffmpeg
#../ffmpeg/configure \
./configure \
	--arch="x86_64" --enable-static --enable-shared \
	--prefix=$PWD/../ffmpeg-out \
	--enable-gpl --enable-swscale \
	--disable-ffserver --disable-ffplay --disable-network --disable-ffmpeg \
	--disable-devices --target-os="mingw32" \
	--enable-w32threads \
	--extra-cflags="-mno-cygwin" \
	--extra-libs="-mno-cygwin" --enable-memalign-hack --disable-mmx

#	--cpu=i686 --disable-static --enable-shared \
#	--enable-memalign-hack --enable-w32threads \ # ok for reading but breaks on writing
# --disable-mmx --enable-w32threads \ # reads and writes ok

#--enable-swscaler  may 9 2008 - does not take this option anymore
#--disable-mmx --enable-w32threads \

#export PATH=/cygdrive/c/Programs/Develop/mingw64/bin:$PATH
#make all install

#cp $(find . -name '*.def') ../ffmpeg-out
#cd ../ffmpeg-out
#rm avutil*.dll
#rm avformat*.dll
#rm avcodec*.dll
#rm swscale*.dll
#mv bin/avutil-50.*.dll ./avutil-50.dll
#mv bin/avformat-52.*.dll ./avformat-52.dll
#mv bin/avcodec-52.*.dll ./avcodec-52.dll
#mv bin/swscale-0.*.dll ./swscale-0.dll


#cp $(find ./include/ffmpeg -name '*.h') ../include/ffmpeg

#echo "
#set PATH=C:\Program Files\Microsoft Visual Studio 9.0\VC\bin;%PATH%
#set PATH=C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE;%PATH%
#lib /machine:x86 /def:avutil-49.def
#lib /machine:x86 /def:avformat-52.def
#lib /machine:x86 /def:avcodec-52.def
#lib /machine:x86 /def:swscale-0.def
#del *.exp
#copy *.dll ..\..\..\libs\vc2008
#copy *.lib ..\..\..\libs\vc2008
#" > makedlls.bat

#chmod a+x makedlls.bat

#./makedlls.bat
