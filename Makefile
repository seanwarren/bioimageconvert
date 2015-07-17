prefix=/usr
BINDIR=$(prefix)/bin
LIBDIR=$(prefix)/lib
MAKEFLAGS += -j 4
LIBBIM=libsrc/libbioimg
LIBRAW=libsrc/libraw
LIBVPX=libsrc/libvpx
LIBX264=libsrc/libx264
LIBX265=libsrc/libx265
FFMPEG=libsrc/ffmpeg
LIBJPEGTURBO=libsrc/libjpeg-turbo
LIBGDCM=libsrc/gdcm
LIBGDCMBIN=libsrc/gdcmbin
LIBS=libs/linux
PKG_CONFIG_PATH=$LIBVPX/vpx.pc:$LIBX264/x264.pc:$LIBX265/build/linux/x265.pc
QMAKEOPTS=
VERSION=2.0.0

all : imgcnv


install:
	install -d $(DESTDIR)$(BINDIR)
	install imgcnv $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install libimgcnv.so.2 $(DESTDIR)$(LIBDIR)


imgcnv:
	-mkdir -p .generated/obj
	
	@echo
	@echo   
	@echo "Building libbioimage in $(LIBBIM)"
	(cd $(LIBBIM); qmake $(QMAKEOPTS) bioimage.pro)
	(cd $(LIBBIM); $(MAKE)) 

	@echo
	@echo
	@echo "Building imgcnv"        
	(cd src; qmake $(QMAKEOPTS) imgcnv.pro)
	(cd src; $(MAKE))

	@echo
	@echo
	@echo "Building libimgcnv"   
	(cd src_dylib; qmake $(QMAKEOPTS) libimgcnv.pro)
	(cd src_dylib; $(MAKE))

full:
	-mkdir -p .generated/obj
	-mkdir -p $(LIBS)

	@echo
	@echo
	@echo "Building libbioimage in $(LIBBIM)"
	(cd $(LIBBIM); qmake $(QMAKEOPTS) bioimage.pro)
	(cd $(LIBBIM); $(MAKE))
	
	@echo
	@echo
	@echo "Building libvpx 1.4.0 in $(LIBVPX)"
	(cd $(LIBVPX); chmod -f u+x configure)
	(cd $(LIBVPX); chmod -f u+x build/make/version.sh)
	(cd $(LIBVPX); chmod -f u+x build/make/rtcd.sh)
	(cd $(LIBVPX); chmod -f u+x build/make/gen_asm_deps.sh)
	(cd $(LIBVPX); chmod -f u+x build/make/gen_asm_deps.sh)     
	(cd $(LIBVPX); ./configure --enable-vp8 --enable-vp9 --enable-pic --disable-examples --disable-unit-tests )
	(cd $(LIBVPX); $(MAKE))
	(cp $(LIBVPX)/libvpx.a $(LIBS)/)
	
	@echo
	@echo
	@echo "Building libx264 20150223 in $(LIBX264)"
	(cd $(LIBX264); chmod -f u+x configure)
	(cd $(LIBX264); chmod -f u+x version.sh)
	(cd $(LIBX264); chmod -f u+x config.guess)
	(cd $(LIBX264); chmod -f u+x config.sub)
	(cd $(LIBX264); ./configure --enable-pic --enable-static --disable-opencl )
	(cd $(LIBX264); $(MAKE))
	(cp $(LIBX264)/libx264.a $(LIBS)/) 
	
	@echo
	@echo
	@echo "Building libx265 1.4 in $(LIBX265)"
	#(cd $(LIBX265)/build/linux; chmod -f u+x make-Makefiles.bash)
	#(cd $(LIBX265)/build/linux; bash make-Makefiles.bash )
	(cd $(LIBX265)/build/linux; cmake -G "Unix Makefiles" ../../source -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPIC )
	(cd $(LIBX265)/build/linux; $(MAKE))
	(cp $(LIBX265)/build/linux/libx265.a $(LIBS)/)

	@echo
	@echo
	@echo "Building libjpeg-turbo 1.4.0 in $(LIBJPEGTURBO)"
	(cd $(LIBJPEGTURBO); chmod -f u+x configure)
	(cd $(LIBJPEGTURBO); chmod -f u+x config.guess)
	(cd $(LIBJPEGTURBO); chmod -f u+x config.sub)
	(cd $(LIBJPEGTURBO); ./configure --enable-pic --enable-static --enable-shared=no )
	(cd $(LIBJPEGTURBO); $(MAKE))
	(cp $(LIBJPEGTURBO)/.libs/libturbojpeg.a $(LIBS)/) 
	
	@echo
	@echo
	@echo "Building libGDCM 2.4.4 in $(LIBGDCM)"
	-mkdir -p $(LIBGDCMBIN)
	(cd $(LIBGDCMBIN); cmake ../gdcm -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPIC -DGDCM_USE_OPENJPEG_V2=ON )
	(cd $(LIBGDCMBIN); $(MAKE))
	(cp $(LIBGDCMBIN)/bin/*.a $(LIBS)/gdcm/)
	
	@echo
	@echo     
	@echo "Building ffmpeg 2.6.3 in $(FFMPEG)"
	(cd $(FFMPEG)/ffmpeg; chmod -f u+x configure)
	(cd $(FFMPEG)/ffmpeg; chmod -f u+x version.sh)
	-mkdir -p $(FFMPEG)/ffmpeg-obj
	(cd $(FFMPEG)/ffmpeg-obj; ../ffmpeg/configure \
		--enable-static --disable-shared --enable-pic --enable-gray \
		--prefix=../ffmpeg-out \
		--extra-cflags="-fPIC -I../../libvpx -I../../libx264 -I../../libx265/source" \
		--extra-cxxflags="-fPIC -I../../libvpx -I../../libx264 -I../../libx265/source" \
		--extra-ldflags="-fPIC -L../../libvpx -L../../libx264 -L../../libx265/build/linux" \
		--enable-gpl --enable-version3 --enable-runtime-cpudetect --enable-pthreads --enable-swscale \
		--disable-ffserver --disable-ffplay --disable-network --disable-ffmpeg --disable-devices \
		--disable-frei0r --disable-libass --disable-libcelt --disable-libopencore-amrnb --disable-libopencore-amrwb \
		--disable-libfreetype --disable-libgsm --disable-libmp3lame --disable-libnut --disable-librtmp \
		--disable-libspeex --disable-libvorbis \
		--enable-bzlib --enable-zlib \
		--disable-libopenjpeg --disable-libschroedinger \
		--enable-libtheora --enable-libvpx --enable-libx264 --enable-encoder=libx264 --enable-libx265 --enable-libxvid \
		--disable-libvo-aacenc --disable-libvo-amrwbenc )
	
	(cd $(FFMPEG)/ffmpeg-obj; $(MAKE) all install)
	
	(cp -f $(FFMPEG)/ffmpeg-out/lib/libavcodec.a $(LIBS)/) 
	(cp -f $(FFMPEG)/ffmpeg-out/lib/libavformat.a $(LIBS)/) 
	(cp -f $(FFMPEG)/ffmpeg-out/lib/libavutil.a $(LIBS)/) 
	(cp -f $(FFMPEG)/ffmpeg-out/lib/libswscale.a $(LIBS)/)  
	-mkdir -p $(FFMPEG)/include
	(cp -Rf $(FFMPEG)/ffmpeg-out/include/libavcodec $(FFMPEG)/include/)
	(cp -Rf $(FFMPEG)/ffmpeg-out/include/libavformat $(FFMPEG)/include/)
	(cp -Rf $(FFMPEG)/ffmpeg-out/include/libavutil $(FFMPEG)/include/)
	(cp -Rf $(FFMPEG)/ffmpeg-out/include/libswscale $(FFMPEG)/include/)
	
	
	@echo
	@echo
	@echo "Building imgcnv"   
	(cd src; qmake $(QMAKEOPTS) imgcnv.pro)
	(cd src; $(MAKE))

	@echo
	@echo
	@echo "Building libimgcnv"   
	(cd src_dylib; qmake $(QMAKEOPTS) libimgcnv.pro)
	(cd src_dylib; $(MAKE))

clean:
	(cd src; $(MAKE) clean)
	rm -rf .generated *.o *~
	rm -rf $(LIBBIM)/.generated $(LIBBIM)/*.o $(LIBBIM)/*~ $(LIBBIM)/.qmake.stash
	(cd $(LIBBIM); $(MAKE) clean)
	(cd $(LIBBIM); $(MAKE) clean)

cleanfull:
	(cd src; $(MAKE) clean)
	rm -rf .generated *.o *~
	rm -rf $(LIBBIM)/.generated $(LIBBIM)/*.o $(LIBBIM)/*~ $(LIBBIM)/.qmake.stash
	(cd $(LIBBIM); $(MAKE) clean)   
	(cd $(LIBVPX); $(MAKE) clean)
	(cd $(LIBX264); $(MAKE) clean)
	rm -rf $(FFMPEG)/ffmpeg-obj
	rm -rf $(FFMPEG)/ffmpeg-out
	#(cd $(FFMPEG)/ffmpeg; $(MAKE) clean)           

realclean: clean
	rm -f imgcnv
	rm -f libimgcnv.so*



.FORCE: imgcnv
