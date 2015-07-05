#!/bin/bash

python download_build_libs.py
cp -f Makefile.linux Makefile
make -j 4
