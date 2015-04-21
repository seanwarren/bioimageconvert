#!/usr/bin/python

# arg 1: path to the directory containing image files

""" imgcnv combine files from a given directory into Z series
"""

__module__    = "imgcnv_create_zt_list.py"
__author__    = "Dmitry Fedorov"
__version__   = "1.0"
__revision__  = "$Rev$"
__date__      = "$Date$"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"


import sys
import os
import math
import re
from operator import itemgetter
from subprocess import Popen, call, PIPE

IMGCNV='imgcnv'

def ensure_trailing_slash(path):
    path = path.replace('\\', '/')
    if not path.endswith('/'):
        return '%s/'%(path)
    else:
        return path

###############################################################
# define needed variables
###############################################################
if len(sys.argv)<2:
    print "usage: PATH_TO_DIR"
    sys.exit()

path = ensure_trailing_slash( sys.argv[1] )

files = os.listdir(path)
files = [os.path.join(path, f) for f in files if f.lower().endswith('.tif')]

###############################################################
# convert every file into pyramidal tiff
###############################################################

for fn in files:
    out = '%s.out.tif'%fn

    # read meta first if input is not pyramidal already
    meta = ''
    command = [IMGCNV, '-i', fn, '-meta-parsed' ]
    try:
        p = Popen (command, stdout=PIPE, stderr=PIPE)
        o,e = p.communicate()
        if p.returncode==0:
            meta = o
    except Exception:
        pass

    if 'image_resolution_level_scales' in meta and 'image_num_resolution_levels' in meta:
        print 'Image %s is already a pyramid, skipping conversion...'%fn
        continue


    command = [IMGCNV, '-i', fn, '-o', out, '-t', 'tiff', '-options', 'compression lzw tiles 512 pyramid subdirs' ]
    print 'Converting: %s'%fn
    #r = Popen (command, stdout=PIPE).communicate()[0]
    try:
        retcode = call (command)
    except Exception:
        retcode = 100

    #if retcode==0 and os.path.getsize(out) >= os.path.getsize(fn):
    if retcode==0:
        os.remove(fn)
        os.rename(out, fn)
