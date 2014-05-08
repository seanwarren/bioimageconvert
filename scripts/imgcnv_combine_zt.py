#!/usr/bin/python 

# arg 1: path to the directory containing image files
# arg 2: X resolution
# arg 3: Y resolution
# arg 4: Z resolution
# arg 5: T resolution

""" imgcnv combine files from a given directory into Z series
"""

__module__    = "imgcnv_combine_zt"
__author__    = "Dmitry Fedorov"
__version__   = "2.0"
__revision__  = "$Rev$"
__date__      = "$Date$"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"


import sys
import os
import math
import re
from operator import itemgetter
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

def break_string_to_alpha_numeric_blocks(s):
    m = re.findall(r'([\d]+|[\D]+)', s)
    def intSafe(x):
        x = x.strip() 
        if x.isdigit(): return int(x)
        return x
    m = map(intSafe, m)
    return m

def reduce2str(l):
  def append(x,y): return str(x)+str(y)
  return reduce(append, l)
  
  
def sort_by_block_alpha_and_numberic(l):  
  
    # split file names into lists of blocks of numbers or non-numbers
    m = map(break_string_to_alpha_numeric_blocks, l)
    
    # get the minimum size of such a block list
    dim = min(map(len, m))
    
    # sort file name blocks block by block
    for i in range(dim):
      m = sorted(m, key=itemgetter(i))
    
    # reduce each list into a file name again  
    return map(reduce2str, m)  
  
  

###############################################################
# define needed variables
###############################################################
if len(sys.argv)<6:
    print "usage: PATH_TO_DIR XRES YRES ZRES TRES NUMZ"
    sys.exit()

path = ensure_trailing_slash( sys.argv[1] )
pl = path.rstrip('/').split('/')
name = pl.pop()
def path_append(x,y): return '%s/%s'%(x,y)
outpath = ensure_trailing_slash(reduce(path_append, pl))

x = float(sys.argv[2])
y = float(sys.argv[3])
z = float(sys.argv[4])
t = float(sys.argv[5])
nz = int(sys.argv[6])

outfile = '%s%s.ome.tif'%(outpath, name)
            

###############################################################
# extract all image files from a given directory
###############################################################

files = os.listdir(path)
files = sort_by_block_alpha_and_numberic(files) 


############################################################### 
# create time series files
###############################################################
num_t = len(files)/nz
num_z = nz
print 'Generating 4D (z: %s, t: %s) image: %s\n'%(num_z, num_t, outfile)

command = [IMGCNV]
for f in files:
    command.append('-i')
    command.append('%s%s'%(path, f))

command.append('-o')
command.append('%s'%(outfile))

command.append('-t')
command.append('ome-tiff')

command.append('-geometry')
command.append('%s,%s'%(num_z, num_t))

command.append('-resolution')
command.append('%s,%s,%s,%s'%(x, y, z, t))

r = Popen (command, stdout=PIPE).communicate()[0]    
