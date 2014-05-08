#!/usr/bin/python 

# arg 1: path to the directory containing image files
# arg 2: X resolution
# arg 3: Y resolution
# arg 4: T resolution

""" imgcnv combine files from a given directory into Time series
"""

__module__    = "imgcnv_combine_t"
__author__    = "Dmitry Fedorov"
__version__   = "1.0"
__revision__  = "$Rev$"
__date__      = "$Date$"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"


import sys
import os
import time
import math
#from copy import deepcopy
#from lxml import etree
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
if len(sys.argv)<5:
    print 'usage: in_image out_image "first_set_of_ops" "second_set_of_ops"'
    sys.exit()


img_in  = sys.argv[1]
img_out = sys.argv[2]
ops1    = sys.argv[3]
ops2    = sys.argv[4]

############################################################### 
# create stats for all BIX files
###############################################################

command = [IMGCNV]
command.append('-i')
command.append(img_in)

command.append('-o')
command.append(img_out)

cmd1 = []
cmd1.extend(command)
cmd1.extend(ops1.split(' '))

cmd2 = []
cmd2.extend(command)
cmd2.extend(ops2.split(' '))


# use wall time for the timing
t0 = time.time()
r = Popen (cmd1, stdout=PIPE).communicate()[0]    
print time.time() - t0, "first process time"

t0 = time.time()
r = Popen (cmd2, stdout=PIPE).communicate()[0]    
print time.time() - t0, "seconds process time"