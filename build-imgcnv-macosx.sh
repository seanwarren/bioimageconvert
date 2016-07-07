#!/bin/bash

python download_build_libs.py
cp -f Makefile.macx Makefile
make -j
