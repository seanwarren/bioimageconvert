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
if len(sys.argv)<2:
    print "usage: PATH_TO_DIR"
    sys.exit()

path = ensure_trailing_slash( sys.argv[1] )
pl = path.rstrip('/').split('/')
name = pl.pop()

###############################################################
# extract all image files from a given directory
###############################################################

files = os.listdir(path)
#files = sort_by_block_alpha_and_numberic(files) 
files.sort()

############################################################### 
# create time series files
###############################################################

f = open('list.txt', 'w')
for n in files:
    fn = '%s%s\n'%(path, n)
    f.write(fn)
f.close()

