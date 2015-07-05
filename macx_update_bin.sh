#!/bin/bash

APPROOT=./imgcnv.app/Contents
APPBIN=$APPROOT/MacOS/imgcnv
APPLIB=$APPROOT/Frameworks
RELLIB=@executable_path/../Frameworks
GCCLIB=/usr/local/lib/gcc/4.9

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
install_name_tool -change $GCCLIB/libgomp.1.dylib $RELLIB/libgomp.1.dylib $APPBIN
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPBIN

# update libs - libimgcnv.dylib @loader_path
#install_name_tool -id $RELLIB/libimgcnv.dylib $APPLIB/libimgcnv.dylib
#install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPLIB/libstdc++.6.dylib

#install_name_tool -change $GCCLIB/libstdc++.6.dylib $RELLIB/libstdc++.6.dylib $APPBIN
#install_name_tool -change $GCCLIB/libgomp.1.dylib $RELLIB/libgomp.1.dylib $APPBIN
#install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPBIN


# update libs - libstdc++.6.dylib
install_name_tool -id $RELLIB/libstdc++.6.dylib $APPLIB/libstdc++.6.dylib
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPLIB/libstdc++.6.dylib

# update libs - libgcc_s.1.dylib
install_name_tool -id $RELLIB/libgcc_s.1.dylib $APPLIB/libgcc_s.1.dylib

# update libs - libgomp.1.dylib
install_name_tool -id $RELLIB/libgomp.1.dylib $APPLIB/libgomp.1.dylib
install_name_tool -change $GCCLIB/libgcc_s.1.dylib $RELLIB/libgcc_s.1.dylib $APPLIB/libgomp.1.dylib


# now let's check their status
file $APPBIN
otool -L $APPBIN

#otool -L $APPLIB/libimgcnv.dylib

otool -L $APPLIB/libstdc++.6.dylib
otool -L $APPLIB/libgomp.1.dylib
otool -L $APPLIB/libgcc_s.1.dylib