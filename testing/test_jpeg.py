#!/usr/bin/python 

# The following command line paratemetrs can be used in any combination:
# all     - execute all tests
# reading - execute reading tests
# writing - execute writing tests
# meta    - execute metadata reading tests
# video   - execute video reading and metadata tests

""" imgcnv testing framework
"""

__module__    = "imgcnv_testing"
__author__    = "Dmitry Fedorov"
__version__   = "1.6"
__revision__  = "$Rev$"
__date__      = "$Date$"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"

import sys
import os
from copy import deepcopy
from subprocess import Popen, call, PIPE
import time
import urllib
import posixpath

IMGCNV = './imgcnv'
IMGCNVVER = '2.0'
url_image_store = 'http://hammer.ece.ucsb.edu/~bisque/test_data/images/' 
local_store_images  = 'images' 
local_store_tests   = 'tests'

failed = 0
passed = 0
results = []

###############################################################
# misc
###############################################################
def version ():
    imgcnvver = Popen ([IMGCNV, '-v'],stdout=PIPE).communicate()[0]
    for line in imgcnvver.splitlines():
        if not line or line.startswith('Input'): return False
        return line.replace('\n', '')
 
def check_version ( needed ):
    inst = version()
    if not inst:            
        raise Exception('imgcnv was not found')
          
    inst_ver = inst.split('.')
    need_ver = needed.split('.')
    if int(inst_ver[0])<int(need_ver[0]) or int(inst_ver[0])==int(need_ver[0]) and int(inst_ver[1])<int(need_ver[1]):
        raise Exception('Imgcnv needs update! Has: '+inst+' Needs: '+needed)   

def parse_imgcnv_info(s):
    d = {}
    for l in s.splitlines():
        k = l.split(': ', 1)
        if len(k)>1:
            d[k[0]] = k[1]
    return d
    
def print_failed(s, f='-'):
    print 'X FAILED %s'%(s)
    global failed
    global results
    failed += 1
    results.append( '%s: %s'%(f,s) )

def print_passed(s):
    print 'PASSED %s'%(s)
    global passed
    passed += 1    

def copy_keys(dict_in, keys_in):
    dict_out = {}
    for k in keys_in:
        if k in dict_in:
            dict_out[k] = dict_in[k]
    return dict_out    
    #return { ?zip(k,v)? for k in keys_in if k in dict_in }   

def fetch_file(filename):
    url = posixpath.join(url_image_store, filename)
    path = os.path.join(local_store_images, filename)  
    if not os.path.exists(path): 
        urllib.urlretrieve(url, path)
    if not os.path.exists(path): 
        print '!!! Could not find required test image: "%s" !!!'%filename
    return path

      
###############################################################
# info comparisons
###############################################################
      
class InfoComparator(object):
    '''Compares two info dictionaries''' 
    def compare(self, iv, tv):
        return False
    def fail(self, k, iv, tv):
        print_failed('%s failed comparison [%s] [%s]'%(k, iv, tv))

class InfoEquality(InfoComparator):
    def compare(self, iv, tv):
        return (iv==tv)
    def fail(self, k, iv, tv):
        print_failed('%s failed comparison %s = %s'%(k, iv, tv))

class InfoNumericLessEqual(InfoComparator):
    def compare(self, iv, tv):
        return (int(iv)<=int(tv))
    def fail(self, k, iv, tv):
        print_failed('%s failed comparison %s <= %s'%(k, iv, tv))


def compare_info(info, test, cc=InfoEquality() ):
    for tk in test:
        if tk not in info:
            print_failed('%s not found in info'%(tk))
            return False;
        if not cc.compare(info[tk], test[tk]):
            cc.fail( tk, info[tk], test[tk] )
            return False;  
    return True
               

###############################################################
# TESTS
###############################################################

def test_image_read( format, filename ):

    print    
    print '---------------------------------------'
    print '%s - %s'%(format, filename)
    print '---------------------------------------'    
  
  
    #------------------------------------------------------------------------
    # reading and converting into TIFF
    #------------------------------------------------------------------------      
    out_name = 'tests/_test_converting_%s.tif'%(filename)
    thumb_name = 'tests/_test_thumbnail_%s.jpg'%(filename)       
    out_fmt = 'tiff'
    filename = 'images/%s'%(filename)

    # test if file can be red
    command = [IMGCNV, '-i', filename, '-info']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_org = parse_imgcnv_info(r)
    
    if r is None or r.startswith('Input format is not supported') or len(info_org)<=0 or 'width' not in info_org or int(info_org['width'])<1:
        print_failed('loading info', format)
        return        
    else:
        print_passed('loading info')

    # convert the file into TIFF   
    command = [IMGCNV, '-i', filename, '-o', out_name, '-t', out_fmt]
    r = Popen (command, stdout=PIPE).communicate()[0]    
        
    # get info from the converted file           
    command = [IMGCNV, '-i', out_name, '-info']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_cnv = parse_imgcnv_info(r)
    
    if r is None or r.startswith('Input format is not supported') or len(info_cnv)<=0:
        print_failed('loading converted info', format)
        return
    else:
        print_passed('loading converted info')
      
    # test if converted file has same info
    info_test = copy_keys(info_cnv, ('pages', 'channels', 'width', 'height', 'depth'))
    if compare_info(info_org, info_test)==True:
        print_passed('geometry')
    
    print    
    

def test_image_write( format, filename ):

    print    
    print '---------------------------------------'
    print '%s - %s'%(format, filename)
    print '---------------------------------------'    
  
    out_name = 'tests/_test_writing_%s.%s'%(filename, format)
    out_fmt = format
    filename = 'images/%s'%(filename) 

    # test if file can be red
    command = [IMGCNV, '-i', filename, '-info']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_org = parse_imgcnv_info(r)
    
    if r is None or r.startswith('Input format is not supported') or len(info_org)<=0:
        print_failed('loading input info', format)
        return        

    # convert the file into format   
    command = [IMGCNV, '-i', filename, '-o', out_name, '-t', out_fmt]
    r = Popen (command, stdout=PIPE).communicate()[0]    
        
    # get info from the converted file           
    command = [IMGCNV, '-i', out_name, '-info']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_cnv = parse_imgcnv_info(r)
    
    if r is None or r.startswith('Input format is not supported') or len(info_cnv)<=0:
        print_failed('loading written info', format)
        return
    else:
        print_passed('loading written info')
      
    # test if converted file has same info
    info_test = copy_keys(info_cnv, ('pages', 'channels', 'width', 'height', 'depth'))
    if compare_info(info_org, info_test)==True:
        print_passed('written geometry')
    
    print       
        

###############################################################
# run tests
###############################################################

check_version ( IMGCNVVER )

try:
    os.mkdir(local_store_images)    
    os.mkdir(local_store_tests)
except:
    pass

mode = sys.argv
if len(mode) <= 1: mode.append('all')

#**************************************************************
# ensure test files are present
#**************************************************************

fetch_file('A01.jpg')
fetch_file('IMG_0184.JPG')
fetch_file('IMG_0488.JPG')
fetch_file('IMG_0562.JPG')
fetch_file('IMG_0593.JPG')
fetch_file('IMG_1003.JPG')
fetch_file('PENTAX_IMGP1618.JPG')


#**************************************************************
# run tests
#**************************************************************

start = time.time()

if 'all' in mode or 'reading' in mode:
    print
    print
    print '***************************************************'
    print 'Reading formats, converting and creating thumbnails'
    print '***************************************************'  
    
    test_image_read( "JPEG", "A01.jpg" )
    test_image_read( "JPEG", "IMG_0184.JPG" )
    test_image_read( "JPEG", "IMG_0488.JPG" )
    test_image_read( "JPEG", "IMG_0562.JPG" )
    test_image_read( "JPEG", "IMG_0593.JPG" )
    test_image_read( "JPEG", "IMG_1003.JPG" )
    test_image_read( "JPEG", "PENTAX_IMGP1618.JPG" )

if 'all' in mode or 'writing' in mode:
    print
    print
    print '***************************************************'
    print 'Writing formats'
    print '***************************************************'  
    
    test_image_write( "JPEG", "A01.jpg" )
    test_image_write( "JPEG", "IMG_0184.JPG" )
    test_image_write( "JPEG", "IMG_0488.JPG" )
    test_image_write( "JPEG", "IMG_0562.JPG" )
    test_image_write( "JPEG", "IMG_0593.JPG" )
    test_image_write( "JPEG", "IMG_1003.JPG" )
    test_image_write( "JPEG", "PENTAX_IMGP1618.JPG" )               


end = time.time()
elapsed= end - start

# print summary
print '\n\nFollowing tests took %s seconds:\n'%(elapsed)

if passed>0 and failed==0:
    print 'Passed all %d tests. Congrats!!!\n'%(passed)
    
if passed>0 and failed>0:
    print 'Passed: %d\n'%(passed)

if failed>0:
    print 'Failed: %d\n'%(failed)
    for s in results:
      print '  > %s'%(s)
