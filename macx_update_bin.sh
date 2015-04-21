#!/bin/sh

APPROOT=./imgcnv.app/Contents
APPBIN=$APPROOT/MacOS/imgcnv

mkdir -p $APPROOT/Libraries
cp /usr/local/lib/gcc/4.9/libstdc++.6.dylib $APPROOT/Libraries/
cp /usr/local/lib/gcc/4.9/libgomp.1.dylib $APPROOT/Libraries/
cp /usr/local/lib/gcc/4.9/libgcc_s.1.dylib $APPROOT/Libraries/
#cp /usr/local/lib/gcc/4.9/libgcc_s.1.dylib $APPROOT/Libraries/

# update app
install_name_tool -change /usr/local/lib/gcc/4.9/libstdc++.6.dylib @executable_path/../Libraries/libstdc++.6.dylib $APPBIN
install_name_tool -change /usr/local/lib/gcc/4.9/libgomp.1.dylib @executable_path/../Libraries/libgomp.1.dylib $APPBIN
install_name_tool -change /usr/local/lib/gcc/4.9/libgcc_s.1.dylib @executable_path/../Libraries/libgcc_s.1.dylib $APPBIN

# update libs - libstdc++.6.dylib
install_name_tool -id @executable_path/../Libraries/libstdc++.6.dylib $APPROOT/Libraries/libstdc++.6.dylib
install_name_tool -change /usr/local/lib/gcc/4.9/libgcc_s.1.dylib @executable_path/../Libraries/libgcc_s.1.dylib $APPROOT/Libraries/libstdc++.6.dylib

# update libs - libgcc_s.1.dylib
install_name_tool -id @executable_path/../Libraries/libgcc_s.1.dylib $APPROOT/Libraries/libgcc_s.1.dylib

# update libs - libgomp.1.dylib
install_name_tool -id @executable_path/../Libraries/libgomp.1.dylib $APPROOT/Libraries/libgomp.1.dylib
install_name_tool -change /usr/local/lib/gcc/4.9/libgcc_s.1.dylib @executable_path/../Libraries/libgcc_s.1.dylib $APPROOT/Libraries/libgomp.1.dylib


# now let's check their status
file $APPBIN
otool -L $APPBIN

otool -L $APPROOT/Libraries/libstdc++.6.dylib
otool -L $APPROOT/Libraries/libgomp.1.dylib
otool -L $APPROOT/Libraries/libgcc_s.1.dylib