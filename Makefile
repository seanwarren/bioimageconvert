prefix=/usr
BINDIR=$(prefix)/bin
MAKEFLAGS += -j 4
LIBBIM=libsrc/libbioimg
LIBRAW=libsrc/libraw
LIBVPX=libsrc/libvpx
LIBX264=libsrc/libx264
FFMPEG=libsrc/ffmpeg
LIBS=libs/linux
QMAKEOPTS=

all : imgcnv


install:
	install imgcnv $(DESTDIR)$(BINDIR)


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
	@echo "Building libvpx in $(LIBVPX)"
	(cd $(LIBVPX); chmod -f u+x configure)
	(cd $(LIBVPX); chmod -f u+x build/make/version.sh)
	(cd $(LIBVPX); chmod -f u+x build/make/rtcd.sh)
	(cd $(LIBVPX); chmod -f u+x build/make/gen_asm_deps.sh)
	(cd $(LIBVPX); chmod -f u+x build/make/gen_asm_deps.sh)     
	(cd $(LIBVPX); ./configure --enable-vp8 --enable-pic --disable-examples)
	(cd $(LIBVPX); $(MAKE))
	(cp $(LIBVPX)/libvpx.a $(LIBS)/)
	
	@echo
	@echo
	@echo "Building libx264 in $(LIBX264)"
	(cd $(LIBX264); chmod -f u+x configure)
	(cd $(LIBX264); chmod -f u+x version.sh)
	(cd $(LIBX264); chmod -f u+x config.guess)
	(cd $(LIBX264); chmod -f u+x config.sub)
	(cd $(LIBX264); ./configure --enable-pic --enable-static --disable-opencl )
	(cd $(LIBX264); $(MAKE))
	(cp $(LIBX264)/libx264.a $(LIBS)/) 
	
	@echo
	@echo     
	@echo "Building ffmpeg in $(FFMPEG)"
	(cd $(FFMPEG)/ffmpeg; chmod -f u+x configure)
	(cd $(FFMPEG)/ffmpeg; chmod -f u+x version.sh)
	-mkdir -p $(FFMPEG)/ffmpeg-obj
	(cd $(FFMPEG)/ffmpeg-obj; ../ffmpeg/configure \
		--enable-static --disable-shared \
		--prefix=./../ffmpeg-out \
		--extra-cflags="-I./../../libvpx -I./../../libx264" \
		--extra-ldflags="-L./../../libvpx -L./../../libx264" \
		--enable-gpl --enable-runtime-cpudetect --enable-pthreads --enable-swscale \
		--disable-ffserver --disable-ffplay --disable-network --disable-ffmpeg --disable-devices \
		--disable-frei0r --disable-libass --disable-libcelt --disable-libopencore-amrnb --disable-libopencore-amrwb \
		--disable-libfreetype --disable-libgsm --disable-libmp3lame --disable-libnut --disable-librtmp \
		--disable-libspeex --disable-libvorbis \
		--enable-bzlib --enable-zlib \
		--disable-libopenjpeg --disable-libschroedinger \
		--enable-libtheora --enable-libvpx --enable-libx264 --enable-libxvid \
		--disable-libvo-aacenc --disable-libvo-amrwbenc \
		--disable-libxavs \
		--disable-vda )
	
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

clean:
	(cd src; $(MAKE) clean)
	rm -rf .generated *.o *~
	(cd $(LIBBIM); $(MAKE) clean)

cleanfull:
	(cd src; $(MAKE) clean)
	rm -rf .generated *.o *~
	(cd $(LIBBIM); $(MAKE) clean)   
	(cd $(LIBVPX); $(MAKE) clean)
	(cd $(LIBX264); $(MAKE) clean)
	rm -rf $(FFMPEG)/ffmpeg-obj
	rm -rf $(FFMPEG)/ffmpeg-out
	#(cd $(FFMPEG)/ffmpeg; $(MAKE) clean)           

realclean: clean
	rm -f imgcnv



.FORCE: imgcnv
