#!/usr/bin/python 

# arg 1: path to the directory containing image files
# arg 2: X resolution
# arg 3: Y resolution
# arg 4: T resolution

""" imgcnv combine files from a given directory into Time series
"""

__module__    = "imgcnv_combine_t"
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
#if len(sys.argv)<4:
#    print "usage: PATH_TO_DIR XRES YRES TRES"
#    sys.exit()

#path = ensure_trailing_slash( sys.argv[1] )
#pl = path.rstrip('/').split('/')
#name = pl.pop()
#def path_append(x,y): return '%s/%s'%(x,y)
#outpath = ensure_trailing_slash(reduce(path_append, pl))

#x = sys.argv[2]
#y = sys.argv[3]
#z = 0.0
#t = sys.argv[4]

#outfile = '%s%s.ome.tif'%(outpath, name)
            

###############################################################
# extract all image files from a given directory
###############################################################

# resolution
x = 0.3096970021724701
y = 0.3096970021724701
z = 0.9750000238418579
t = 0

ch_start = 0
ch_end   = 2
page_start = 1
page_end   = 26
file_name_template = 'ch%d_pages_%.6d.tif'
file_inter_template = 'page_%.6d.tif'

outfile = 'image.ome.tif'

files_ch = []
for ch in range(ch_start, ch_end+1):
    f = []
    for i in range(page_start, page_end+1):
        f.append( file_name_template%(ch, i) )
    files_ch.append(f)


inter = []
for i in range(page_start, page_end+1):
    inter.append( file_inter_template%(i) )

#files = os.listdir(path)
#files = sort_by_block_alpha_and_numberic(files) 

############################################################### 
# combine channels
###############################################################

#imgcnv -i ch0_pages_000001.tif -c ch1_pages_000001.tif -c ch2_pages_000001.tif -o page_0001.tif -t tiff

for i in range(page_start, page_end+1):
    command = [IMGCNV]

    command.append('-i')
    command.append(files_ch[0][i-1])
    
    for ch in range(ch_start+1, ch_end+1):
        command.append('-c')
        command.append( files_ch[ch][i-1] )
    
    command.append('-t')
    command.append('tiff')
    
    command.append('-o')
    command.append(inter[i-1])
    
    #print ' '.join(command)
    r = Popen (command, stdout=PIPE).communicate()[0]    


############################################################### 
# create time series files
###############################################################


num_z = len(inter)
num_t = 1
print 'Generating (%d time points series): %s\n'%(num_z, outfile)

command = [IMGCNV]
for f in inter:
    command.append('-i')
    command.append(f)

command.append('-o')
command.append('%s'%(outfile))

command.append('-t')
command.append('ome-tiff')

command.append('-geometry')
command.append('%s,%s'%(num_z, num_t))

command.append('-resolution')
command.append('%s,%s,%s,%s'%(x, y, z, t))

r = Popen (command, stdout=PIPE).communicate()[0]
#print ' '.join(command)
