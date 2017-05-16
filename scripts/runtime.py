#!/usr/bin/python 

# arg 1: path to the directory containing image files
# arg 2: X resolution
# arg 3: Y resolution
# arg 4: T resolution

""" imgcnv combine files from a given directory into Time series
"""

__module__    = "time"
__author__    = "Dmitry Fedorov"
__version__   = "1.0"
__revision__  = "$Rev$"
__date__      = "$Date$"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"


import sys
import os
import time
import math
from subprocess import Popen, call, PIPE

IMGCNV='imgcnv'
           

###############################################################
# define needed variables
###############################################################
if len(sys.argv)<1:
    print 'usage: time ops_as_for_imgcnv'
    sys.exit()


############################################################### 
# create stats for all BIX files
###############################################################

command = [IMGCNV]
command.extend(sys.argv)


# use wall time for the timing
t0 = time.time()
r = Popen (command, stdout=PIPE).communicate()[0]    
print time.time() - t0
