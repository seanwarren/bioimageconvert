#!/bin/bash

GCCLIB=/usr/local/lib/gcc/4.9
GCCLIBO=/usr/local/opt/gcc49/lib/gcc/4.9 # optional location that may be linked by ld

#####################################################
# application
#####################################################
echo
echo APP

APPROOT=./imgcnv.app/Contents
APPBIN=$APPROOT/MacOS/imgcnv
APPLIB=$APPROOT/Frameworks
RELLIB=@executable_path/../Frameworks


mkdir -p $APPLIB
cp -n $GCCLIB/libstdc++.6.dylib $APPLIB/
cp -n $GCCLIB/libgomp.1.dylib $APPLIB/
cp -n $GCCLIB/libgcc_s.1.dylib $APPLIB/

#cp libimgcnv.dylib $APPLIB/

chmod 664 $APPLIB/libstdc++.6.dylib
chmod 664 $APPLIB/libgomp.1.dylib
chmod 664 $APPLIB/libgcc_s.1.dylib

# update app
install_name_tool -change $GCCLIB/libstdc++.6.dylib $RELLIB/libstdc++.6.dylib $APPBIN
install_name_tool -change $GCCLIBO/libstdc++.6.dylib $RELLIB/libstdc++.6.dylib $APPBIN

install_name_tool -change $GCCLIB/libgomp.1.dylib $RELLIB/libgomp.1.dylib $APPBIN
install_name_tool -change $GCCLIBO/libgomp.1.dylib $RELLIB/libgomp.1.dylib $APPBIN

install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPBIN
install_name_tool -change $GCCLIBO/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPBIN

# update libs - libstdc++.6.dylib
install_name_tool -id $RELLIB/libstdc++.6.dylib $APPLIB/libstdc++.6.dylib
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPLIB/libstdc++.6.dylib

# update libs - libgcc_s.1.dylib
install_name_tool -id $RELLIB/libgcc_s.1.dylib $APPLIB/libgcc_s.1.dylib

# update libs - libgomp.1.dylib
install_name_tool -id $RELLIB/libgomp.1.dylib $APPLIB/libgomp.1.dylib
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPLIB/libgomp.1.dylib


#####################################################
# dylib
#####################################################
echo
echo DYLIB

LIBPATH=./libimgcnv.bundle
LIBBIN=$LIBPATH/libimgcnv.dylib
RELLIB=@loader_path

mkdir -p $LIBPATH
cp libimgcnv.dylib $LIBPATH/
cp -n $GCCLIB/libstdc++.6.dylib $LIBPATH/
cp -n $GCCLIB/libgomp.1.dylib $LIBPATH/
cp -n $GCCLIB/libgcc_s.1.dylib $LIBPATH/

chmod 664 $LIBPATH/libstdc++.6.dylib
chmod 664 $LIBPATH/libgomp.1.dylib
chmod 664 $LIBPATH/libgcc_s.1.dylib

# update dylib
install_name_tool -change $GCCLIB/libstdc++.6.dylib $RELLIB/libstdc++.6.dylib $LIBBIN
install_name_tool -change $GCCLIBO/libstdc++.6.dylib $RELLIB/libstdc++.6.dylib $LIBBIN

install_name_tool -change $GCCLIB/libgomp.1.dylib $RELLIB/libgomp.1.dylib $LIBBIN
install_name_tool -change $GCCLIBO/libgomp.1.dylib $RELLIB/libgomp.1.dylib $LIBBIN

install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $LIBBIN
install_name_tool -change $GCCLIBO/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $LIBBIN

# update libs - libstdc++.6.dylib
install_name_tool -id $RELLIB/libstdc++.6.dylib $LIBPATH/libstdc++.6.dylib
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $LIBPATH/libstdc++.6.dylib

# update libs - libgcc_s.1.dylib
install_name_tool -id $RELLIB/libgcc_s.1.dylib $LIBPATH/libgcc_s.1.dylib

# update libs - libgomp.1.dylib
install_name_tool -id $RELLIB/libgomp.1.dylib $LIBPATH/libgomp.1.dylib
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $LIBPATH/libgomp.1.dylib




#####################################################
# now let's check their status
#####################################################

file $APPBIN
otool -L $APPBIN

otool -L $APPLIB/libstdc++.6.dylib
otool -L $APPLIB/libgomp.1.dylib
otool -L $APPLIB/libgcc_s.1.dylib


file $LIBBIN
otool -L $LIBBIN

otool -L $LIBPATH/libstdc++.6.dylib
otool -L $LIBPATH/libgomp.1.dylib
otool -L $LIBPATH/libgcc_s.1.dylib