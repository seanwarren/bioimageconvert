#!/bin/bash

chmod a+x ./ffmpeg/configure
chmod a+x ./ffmpeg/version.sh

rm -Rf ./ffmpeg-obj
rm -Rf ./ffmpeg-out

#####################################################
# The default compile is now Intel
#####################################################

export MACOSX_DEPLOYMENT_TARGET=10.7

#PWD= `pwd`
#export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PWD/../libvpx/vpx.pc:$PWD/../x264/x264.pc:$PWD/../x265/build/linux/x265.pc

mkdir -p ffmpeg-obj
cd ffmpeg-obj
../ffmpeg/configure \
  --enable-static --disable-shared --as=yasm \
  --prefix=./../ffmpeg-out \
  #--extra-cflags="-I./../../libvpx -I./../../libx264 -I./../../libx265/source" \
  --enable-gpl --enable-runtime-cpudetect --enable-pthreads --enable-swscale \
  --disable-ffserver --disable-ffplay --disable-network --disable-ffmpeg --disable-devices \
  --disable-frei0r --disable-libass --disable-libcelt --disable-libopencore-amrnb --disable-libopencore-amrwb \
  --disable-libfreetype --disable-libgsm --disable-libmp3lame --disable-libnut --disable-librtmp \
  --disable-libspeex --disable-libvorbis \
  --enable-bzlib --enable-zlib \
  --disable-libopenjpeg --disable-libschroedinger \
  --enable-libtheora --enable-libvpx --enable-libx264 --enable-libx265 --enable-libxvid \
  --disable-libvo-aacenc --disable-libvo-amrwbenc \
  --disable-libxavs \
  --disable-vda --disable-iconv
 

make all install -j 8

cd ..

mkdir -p ./include
cp -Rf ./ffmpeg-out/include/libavcodec ./include/
cp -Rf ./ffmpeg-out/include/libavformat ./include/
cp -Rf ./ffmpeg-out/include/libavutil ./include/
cp -Rf ./ffmpeg-out/include/libswscale ./include/
cp -Rf ./ffmpeg-out/include/libswresample ./include/

mkdir -p ../../libs
mkdir -p ../../libs/macosx

cp -f ./ffmpeg-out/lib/libavcodec.a ../../libs/macosx/libavcodec.a
cp -f ./ffmpeg-out/lib/libavformat.a ../../libs/macosx/libavformat.a
cp -f ./ffmpeg-out/lib/libavutil.a ../../libs/macosx/libavutil.a
cp -f ./ffmpeg-out/lib/libswscale.a ../../libs/macosx/libswscale.a
cp -f ./ffmpeg-out/lib/libswresample.a ../../libs/macosx/libswresample.a
