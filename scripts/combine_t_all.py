#!/usr/bin/python 

import sys
import os
from subprocess import Popen, call, PIPE
 
def ensure_trailing_slash(path):
    path = path.replace('\\', '/')
    if not path.endswith('/'):
        return '%s/'%(path)
    else:
        return path

           

###############################################################
# extract all BIX files from a given directory
###############################################################
if len(sys.argv)<4:
    print "usage: PATH_TO_DIR XRES YRES TRES"
    sys.exit()

path = ensure_trailing_slash( sys.argv[1] )
dirs = os.listdir(path)
dirs.sort()

x = sys.argv[2]
y = sys.argv[3]
z = 0.0
t = sys.argv[4]

script = sys.argv[0].replace( 'combine_t_all.py', 'imgcnv_combine_t.py' )

# run all scripts
for d in dirs:
    path1 = '%s%s/'%(path , d)
    print 'Running script for '+path1
    Popen (['python', script, path1, x, y, t],stdout=PIPE).communicate()[0]


        
    