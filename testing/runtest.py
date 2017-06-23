#!/usr/bin/python

# The following command line paratemetrs can be used in any combination:
# all     - execute all tests
# reading - execute reading tests
# writing - execute writing tests
# meta    - execute metadata reading tests
# video   - execute video reading and metadata tests

""" imgcnv testing framework
"""

__author__    = "Dmitry Fedorov"
__version__   = "2.0.9"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"

import sys
import os
from copy import deepcopy
from subprocess import Popen, call, PIPE
import time
import urllib
import posixpath

IMGCNV = './imgcnv'
IMGCNVVER = __version__
url_image_store = 'http://biodev.ece.ucsb.edu/~bisque/test_data/images/'
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
        try:
            if isinstance(tv, (int, long)):
                return (int(iv)==tv)
            if isinstance(tv, (float)):
                return (float(iv)==tv)
        except:
            pass
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

    #------------------------------------------------------------------------
    # Writing thumbnail
    #------------------------------------------------------------------------
    command = [IMGCNV, '-i', filename, '-o', thumb_name, '-t', 'jpeg', '-depth', '8,d', '-page', '1', '-display', '-resize', '128,128,BC,AR']
    r = Popen (command, stdout=PIPE).communicate()[0]

    command = [IMGCNV, '-i', thumb_name, '-info']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_thb = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(info_thb)<=0:
        print_failed('loading thumbnail info', format)
        return
    else:
        print_passed('loading thumbnail info')

    if compare_info( info_thb, {'pages':'1', 'channels':'3', 'depth':'8'} )==True:
        if compare_info(info_thb, {'width':'128', 'height':'128'}, InfoNumericLessEqual())==True:
            print_passed('thumbnail geometry')

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


def test_video_write( format, filename ):

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
    command = [IMGCNV, '-i', filename, '-o', out_name, '-t', out_fmt, '-depth', '8,d']
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
    info_test = copy_keys(info_cnv, ('width', 'height'))
    if compare_info(info_org, info_test)==True:
        print_passed('written geometry')

    print


def test_image_metadata( format, filename, meta_test, meta_test_cnv=None  ):

    print
    print '---------------------------------------'
    print '%s - %s'%(format, filename)
    print '---------------------------------------'

    out_name = 'tests/_test_metadata_%s.ome.tif'%(filename)
    out_fmt = 'ome-tiff'
    filename = 'images/%s'%(filename)

    # test if file can be red
    command = [IMGCNV, '-i', filename, '-meta-parsed']
    r = Popen (command, stdout=PIPE).communicate()[0]
    meta_org = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(meta_org)<=0:
        print_failed('loading metadata', format)
        return

    #print str(meta_org)

    # test if converted file has same info
    if compare_info(meta_org, meta_test)==True:
        print_passed('reading metadata')

    # convert the file into format
    command = [IMGCNV, '-i', filename, '-o', out_name, '-t', out_fmt]
    r = Popen (command, stdout=PIPE).communicate()[0]

    # get info from the converted file
    command = [IMGCNV, '-i', out_name, '-meta-parsed']
    r = Popen (command, stdout=PIPE).communicate()[0]
    meta_cnv = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(meta_cnv)<=0:
        print_failed('loading written metadata', format)
        return
    else:
        print_passed('loading written metadata')

    #print str(meta_cnv)

    if meta_test_cnv is None: meta_test_cnv=meta_test
    if compare_info(meta_cnv, meta_test_cnv)==True:
        print_passed('writing metadata')

    print

def test_metadata_read( format, filename, meta_test  ):

    print
    print '---------------------------------------'
    print '%s - %s'%(format, filename)
    print '---------------------------------------'

    filename = 'images/%s'%(filename)

    # test if file can be red
    command = [IMGCNV, '-i', filename, '-meta']
    r = Popen (command, stdout=PIPE).communicate()[0]
    meta_org = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(meta_org)<=0:
        print_failed('reading meta-data', format)
        return

    # test if converted file has same info
    if compare_info(meta_org, meta_test)==True:
        print_passed('reading meta-data')

    print

def test_image_video( format, filename, meta_test  ):

    print
    print '---------------------------------------'
    print '%s - %s'%(format, filename)
    print '---------------------------------------'

    out_name = 'tests/_test_metadata_%s.ome.tif'%(filename)
    out_fmt = 'ome-tiff'
    filename = 'images/%s'%(filename)

    # test if file can be red
    command = [IMGCNV, '-i', filename, '-meta-parsed']
    r = Popen (command, stdout=PIPE).communicate()[0]
    meta_org = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(meta_org)<=0:
        print_failed('reading video', format)
        return

    #print str(meta_org)

    # test if converted file has same info
    if compare_info(meta_org, meta_test)==True:
        print_passed('reading video info')

    print

def test_image_transforms( transform_type, transform, filename, meta_test  ):

    print
    print '---------------------------------------'
    print '%s - %s - %s'%(transform_type, transform, filename)
    print '---------------------------------------'

    format = 'tiff'
    out_name = 'tests/_test_transform_%s%s_%s.%s'%(filename, transform_type, transform, format)
    out_fmt = format
    filename = 'images/%s'%(filename)

    # convert the file into transform
    command = [IMGCNV, '-i', filename, '-o', out_name, '-t', out_fmt, transform_type, transform]
    r = Popen (command, stdout=PIPE).communicate()[0]

    # get info from the converted file
    command = [IMGCNV, '-i', out_name, '-meta-parsed']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_cnv = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(info_cnv)<=0:
        print_failed('loading written info for', transform)
        return

    # test if converted file has same info
    if compare_info(info_cnv, meta_test)==True:
        print_passed('reading transformed info')

    print


def test_image_commands( extra, filename, meta_test  ):

    print
    print '---------------------------------------'
    print '%s - %s'%(extra, filename)
    print '---------------------------------------'

    format = 'tiff'
    out_name = 'tests/_test_command_%s_%s.%s'%(filename, '_'.join(extra), format)
    out_fmt = format
    filename = 'images/%s'%(filename)

    # convert the file into transform
    command = [IMGCNV, '-i', filename, '-o', out_name]
    if '-t' not in extra:
        command.extend(['-t', out_fmt])
    command.extend(extra)
    r = Popen (command, stdout=PIPE).communicate()[0]

    # get info from the converted file
    command = [IMGCNV, '-i', out_name, '-meta-parsed']
    r = Popen (command, stdout=PIPE).communicate()[0]
    info_cnv = parse_imgcnv_info(r)

    if r is None or r.startswith('Input format is not supported') or len(info_cnv)<=0:
        print_failed('loading written info for ', extra)
        return

    # test if converted file has same info
    if compare_info(info_cnv, meta_test)==True:
        print_passed('reading command info')

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

fetch_file('0022.zvi')
fetch_file('040130Topography001.tif')
fetch_file('107_07661.bmp')
fetch_file('112811B_5.oib')
fetch_file('122906_3Ax(inverted).avi')
fetch_file('161pkcvampz1Live2-17-2004_11-57-21_AM.tif')
fetch_file('23D3HA-cy3 psd-gfp-488 Homer-647 DIV 14 - 3.zvi')
fetch_file('241aligned.avi')
fetch_file('3Dstack.tif.3D.mov')
fetch_file('7.5tax10.stk')
fetch_file('A01.jpg')
fetch_file('autocorrelation.tif')
fetch_file('AXONEME.002')
fetch_file('B01C0201.M2T')
fetch_file('B4nf.RS.RE.z1.5.15.06.3DAnimation.avi')
fetch_file('bigtiff.ome.btf')
fetch_file('cells.ome.tif')
fetch_file('combinedsubtractions.lsm')
fetch_file('CRW_0136.CRW')
fetch_file('CRW_0136_COMPR.dng')
fetch_file('CSR 4 mo COX g M cone r PNA b z1.oib')
fetch_file('DSCN0041.NEF')
fetch_file('EleanorRigby.mpg')
fetch_file('flowers_24bit_hsv.tif')
fetch_file('flowers_24bit_nointr.png')
fetch_file('flowers_8bit_gray.png')
fetch_file('flowers_8bit_palette.png')
fetch_file('Girsh_path3.m2ts')
fetch_file('HEK293_Triple_Dish1_set_9.lsm')
fetch_file('IMG_0040.CR2')
fetch_file('IMG_0184.JPG')
fetch_file('IMG_0488.JPG')
fetch_file('IMG_0562.JPG')
fetch_file('IMG_0593.JPG')
fetch_file('IMG_1003.JPG')
fetch_file('K560-tax-6-7-7-1.stk')
fetch_file('Live_10-3-2009_6-44-12_PM.tif')
fetch_file('MB_10X_20100303.oib')
fetch_file('MDD2-7.stk')
fetch_file('MF Mon 2x2.tif')
fetch_file('Muller cell z4.oib.3D.mov')
fetch_file('MZ2.PIC')
fetch_file('out.avi')
fetch_file('out.flv')
fetch_file('out.m4v')
fetch_file('out.mjpg')
fetch_file('out.mkv')
fetch_file('out.mov')
fetch_file('out.mpg')
fetch_file('out.ogg')
fetch_file('out.swf')
fetch_file('out.vcd')
fetch_file('out.webm')
fetch_file('out.wmv')
fetch_file('out_h264.mp4')
fetch_file('out_h265.mp4')
fetch_file('P1110010.ORF')
fetch_file('PENTAX_IMGP1618.JPG')
fetch_file('PICT1694.MRW')
fetch_file('radiolaria.avi')
fetch_file('Retina 4 top.oib')
fetch_file('Step_into_Liquid_1080.wmv')
fetch_file('sxn3_w1RGB-488nm_s1.stk')
fetch_file('test z1024 Image0004.oib')
fetch_file('test.ogv')
fetch_file('test16bit.btf')
fetch_file('tubule20000.ibw')
fetch_file('Untitled_MMImages_Pos0.ome.tif')
fetch_file('wta.ome.tif')
fetch_file('monument_imgcnv.256.tif')
fetch_file('monument_imgcnv.ome.tif')
fetch_file('monument_imgcnv_subdirs.tif')
fetch_file('monument_imgcnv_topdirs.tif')
fetch_file('monument_photoshop.tif')
fetch_file('monument_vips.tif')
fetch_file('10')
fetch_file('MR-MONO2-8-16x-heart')
fetch_file('US-MONO2-8-8x-execho')
fetch_file('US-PAL-8-10x-echo')
fetch_file('0015.DCM')
fetch_file('0020.DCM')
fetch_file('ADNI_002_S_0295_MR_3-plane_localizer__br_raw_20060418193538653_1_S13402_I13712.dcm')
fetch_file('BetSog_20040312_Goebel_C2-0001-0001-0001.dcm')
fetch_file('IM-0001-0001.dcm')
fetch_file('test4.tif')

fetch_file('16_day1_1_patient_29C93FK6_1.nii')
fetch_file('filtered_func_data.nii')
fetch_file('newsirp_final_XML.nii')
fetch_file('avg152T1_LR_nifti.hdr')
fetch_file('avg152T1_LR_nifti.img')
fetch_file('T1w.nii.gz')
fetch_file('219j_q050.jxr')
fetch_file('219j_q100.jxr')
fetch_file('219j_q080.webp')
fetch_file('219j_q100.webp')
fetch_file('219j.jp2')

fetch_file('IMG_1913_16bit_prophoto_ts1024_q90.jp2')
fetch_file('retina.jp2')
fetch_file('IMG_1913_16bit_prophoto_q90.jxr')
fetch_file('IMG_1913_prophoto_q90.webp')

fetch_file('6J0A3548.CR2')
fetch_file('IMG_0184_RGBA.png')

fetch_file('20150917_05195_DNA-TET-25k-DE20_raw.region_000.sum-all_003-072.mrc')
fetch_file('golgi.mrc')
fetch_file('Tile_19491580_0_1.mrc')
fetch_file('dual.rec')


#**************************************************************
# run tests
#**************************************************************

start = time.time()

if 'all' in mode or 'reading' in mode:
    print
    print
    print '***************************************************'
    print '"reading" - Reading formats, converting and creating thumbnails'
    print '***************************************************'

    test_image_read( "BMP", "107_07661.bmp" )
    test_image_read( "JPEG", "A01.jpg" )
    test_image_read( "PNG", "flowers_24bit_nointr.png" )
    test_image_read( "PNG", "flowers_8bit_gray.png" )
    test_image_read( "PNG", "flowers_8bit_palette.png" )
    test_image_read( "BIORAD-PIC", "MZ2.PIC" )
    test_image_read( "FLUOVIEW", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_image_read( "Zeiss LSM", "combinedsubtractions.lsm" )
    test_image_read( "Zeiss LSM 1 ch", "HEK293_Triple_Dish1_set_9.lsm" )
    test_image_read( "OME-TIFF", "wta.ome.tif" )
    test_image_read( "TIFF float", "autocorrelation.tif" )
    test_image_read( "STK", "K560-tax-6-7-7-1.stk" )
    test_image_read( "STK", "MDD2-7.stk" )
    test_image_read( "STK", "sxn3_w1RGB-488nm_s1.stk" )
    test_image_read( "STK", "7.5tax10.stk" )
    test_image_read( "OIB", "test z1024 Image0004.oib" )
    test_image_read( "OIB", "MB_10X_20100303.oib" )
    test_image_read( "NANOSCOPE", "AXONEME.002" )
    test_image_read( "IBW", "tubule20000.ibw" )
    test_image_read( "PSIA", "040130Topography001.tif" )
    test_image_read( "BigTIFF", "test16bit.btf" )
    test_image_read( "OME-BigTIFF", "bigtiff.ome.btf" )
    test_image_read( "ZVI", "23D3HA-cy3 psd-gfp-488 Homer-647 DIV 14 - 3.zvi" )
    test_image_read( "ZVI", "0022.zvi" )
    test_image_read( "Adobe DNG", "CRW_0136_COMPR.dng" )

    test_image_read( "JPEG-XR", "219j_q050.jxr" )
    test_image_read( "JPEG-XR LOSSLESS", "219j_q100.jxr" )
    test_image_read( "JPEG-2000", "219j.jp2" )
    test_image_read( "WEBP", "219j_q080.webp" )
    test_image_read( "WEBP LOSSLESS", "219j_q100.webp" )

    # DICOM
    test_image_read( "DICOM", "10" )
    test_image_read( "DICOM", "0015.DCM" )
    test_image_read( "DICOM", "IM-0001-0001.dcm" )

    # INFTI
    test_image_read( "NIFTI", "16_day1_1_patient_29C93FK6_1.nii" )
    test_image_read( "NIFTI", "filtered_func_data.nii" )
    test_image_read( "NIFTI", "newsirp_final_XML.nii" )
    test_image_read( "NIFTI", "avg152T1_LR_nifti.hdr" )
    test_image_read( "NIFTI", "T1w.nii.gz" )

    # video formats
    test_image_read( "QuickTime", "3Dstack.tif.3D.mov" )
    test_image_read( "AVI", "radiolaria.avi" )
    test_image_read( "OGG", "test.ogv" )
    test_image_read( "FLV", "out.flv" )
    test_image_read( "WMV", "out.wmv" )
    test_image_read( "VCD", "out.vcd" )
    test_image_read( "MJPEG", "out.mjpg" )
    test_image_read( "Matroska", "out.mkv" )
    test_image_read( "MPEG1", "EleanorRigby.mpg" )
    test_image_read( "MPEG4", "out.m4v" )
    test_image_read( "H264", "out_h264.mp4" )
    test_image_read( "H265", "out_h265.mp4" )
    test_image_read( "WebM", "out.webm" )

    # MRC
    test_image_read( "MRC", "Tile_19491580_0_1.mrc" )
    test_image_read( "MRC", "golgi.mrc" )


if 'all' in mode or 'writing' in mode:
    print
    print
    print '***************************************************'
    print '"writing" - Writing formats'
    print '***************************************************'

    test_image_write( "JPEG", "flowers_24bit_nointr.png" )
    test_image_write( "PNG", "flowers_24bit_nointr.png" )
    test_image_write( "BMP", "flowers_24bit_nointr.png" )
    test_image_write( "TIFF", "flowers_24bit_nointr.png" )
    test_image_write( "OME-TIFF", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_image_write( "BigTIFF", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_image_write( "OME-BigTIFF", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_image_write( "JXR", "flowers_24bit_nointr.png" )
    test_image_write( "WEBP", "flowers_24bit_nointr.png" )
    test_image_write( "JP2", "flowers_24bit_nointr.png" )

if 'all' in mode or 'writingvideo' in mode:
    print
    print
    print '***************************************************'
    print '"writingvideo" - Writing video'
    print '***************************************************'

    test_video_write( "AVI", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "QuickTime", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "MPEG4", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "WMV", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "OGG", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "Matroska", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "FLV", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "WEBM", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "WEBM9", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "H264", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )
    test_video_write( "H265", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif" )

#sys.exit (0)

if 'all' in mode or 'meta' in mode:
    print
    print
    print '***************************************************'
    print '"meta" - Reading and converting metadata'
    print '***************************************************'

    meta_test = {}
    meta_test['image_num_z'] = '6'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.192406'
    meta_test['pixel_resolution_y'] = '0.192406'
    meta_test['pixel_resolution_z'] = '1.185000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    test_image_metadata( "PIC", "MZ2.PIC", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '13'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.207160'
    meta_test['pixel_resolution_y'] = '0.207160'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['channel_0_name'] = 'FITC'
    meta_test['channel_1_name'] = 'Cy3'
    # the following fields will not be tested for conversion into OME-TIFF
    meta_test_full = deepcopy(meta_test)
    meta_test_full['objective/name'] = 'UPLAPO 40XO'
    meta_test_full['objective/magnification'] = 40.0
    test_image_metadata( "TIFF Fluoview", "161pkcvampz1Live2-17-2004_11-57-21_AM.tif", meta_test_full, meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '30'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.138661'
    meta_test['pixel_resolution_y'] = '0.138661'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    # the following fields will not be tested for conversion into OME-TIFF
    meta_test_full = deepcopy(meta_test)
    meta_test_full['objective/name'] = 'Achroplan 63x/0.95 W'
    meta_test_full['objective/magnification'] = 63.0
    meta_test_full['objective/numerical_aperture'] = 0.95
    meta_test_full['channels/channel_00000/color'] = '0,255,0'
    meta_test_full['channels/channel_00000/lsm_excitation_wavelength'] = 514.0
    meta_test_full['channels/channel_00000/lsm_pinhole_radius'] = 185.0
    meta_test_full['channels/channel_00000/name'] = 'Ch3-T3'
    meta_test_full['channels/channel_00000/power'] = 38.8
    meta_test_full['channels/channel_00001/color'] = '255,0,0'
    meta_test_full['channels/channel_00001/lsm_excitation_wavelength'] = 458.0
    meta_test_full['channels/channel_00001/lsm_pinhole_radius'] = 255.5
    meta_test_full['channels/channel_00001/name'] = 'Ch2-T4'
    meta_test_full['channels/channel_00001/power'] = 63.5
    test_image_metadata( "TIFF Zeiss LSM", "combinedsubtractions.lsm", meta_test_full, meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '152'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.124000'
    meta_test['pixel_resolution_y'] = '0.124000'
    meta_test['pixel_resolution_z'] = '0.350000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['channel_0_name'] = 'Alexa Fluor 488 - 488nm'
    meta_test['channel_1_name'] = 'Alexa Fluor 546 - 543nm'
    meta_test['channel_2_name'] = 'DRAQ5 - 633nm'
    # the following fields will not be tested for conversion into OME-TIFF
    meta_test_full = deepcopy(meta_test)
    meta_test_full['objective/name'] = 'UPLFLN    40X O  NA:1.30'
    meta_test_full['objective/magnification'] = 40.0
    meta_test_full['objective/numerical_aperture'] = 1.3
    test_image_metadata( "OME-TIFF", "wta.ome.tif", meta_test_full, meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '31'
    meta_test['pixel_resolution_t'] = '4.000000'
    test_image_metadata( "STK", "K560-tax-6-7-7-1.stk", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '105'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.430000'
    meta_test['pixel_resolution_y'] = '0.430000'
    meta_test['pixel_resolution_z'] = '0.488000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    # the following fields will not be tested for conversion into OME-TIFF
    meta_test_full = deepcopy(meta_test)
    meta_test_full['stage_distance_z'] = '0.488000'
    meta_test_full['stage_position_x'] = '3712.200000'
    meta_test_full['stage_position_y'] = '-2970.340000'
    meta_test_full['stage_position_z'] = '25.252000'
    test_image_metadata( "STK", "sxn3_w1RGB-488nm_s1.stk", meta_test_full, meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '16'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '2.483410'
    meta_test['pixel_resolution_y'] = '2.480193'
    meta_test['pixel_resolution_z'] = '0.937500'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['channel_0_name'] = 'Alexa Fluor 405'
    meta_test['channel_1_name'] = 'Cy3'
    test_image_metadata( "OIB", "MB_10X_20100303.oib", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.177539'
    meta_test['pixel_resolution_y'] = '0.177539'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    test_image_metadata( "PSIA", "040130Topography001.tif", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    #meta_test['image_num_p'] = '2'
    meta_test['pixel_resolution_x'] = '0.058594'
    meta_test['pixel_resolution_y'] = '0.058594'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    test_image_metadata( "NANOSCOPE", "AXONEME.002", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    #meta_test['image_num_p'] = '3'
    test_image_metadata( "IBW", "tubule20000.ibw", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '13'
    meta_test['image_num_t'] = '1'
    meta_test['pixel_resolution_x'] = '0.207160'
    meta_test['pixel_resolution_y'] = '0.207160'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['channel_0_name'] = 'FITC'
    meta_test['channel_1_name'] = 'Cy3'
    test_image_metadata( "OME-BigTIFF", "bigtiff.ome.btf", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_c'] = '1'
    meta_test['image_pixel_depth'] = '32'
    meta_test['image_pixel_format'] = 'floating point'
    test_image_metadata( "TIFF float", "autocorrelation.tif", meta_test )


if 'all' in mode or 'readmeta' in mode:
    print
    print
    print '***************************************************'
    print '"readmeta" - Reading and parsing metadata'
    print '***************************************************'

    # testing extracting GPS tags from EXIF
    meta_test = {}
    meta_test['Exif/GPSInfo/GPSLatitude']    = '34deg 26.27000\''
    meta_test['Exif/GPSInfo/GPSLatitudeRef'] = 'North'
    test_metadata_read( "JPEG EXIF", "IMG_0488.JPG", meta_test )

    # testing extracting IPTC tags
    meta_test = {}
    meta_test['Iptc/Application2/City']    = 'Santa Barbara'
    meta_test['Iptc/Application2/ProvinceState'] = 'CA'
    test_metadata_read( "JPEG IPTC", "IMG_0184.JPG", meta_test )

    # testing reading RGBA PNG image
    meta_test = {}
    meta_test['image_mode'] = 'RGBA'
    meta_test['ColorProfile/color_space'] = 'RGB'
    meta_test['ColorProfile/description'] = 'sRGB IEC61966-2.1'
    meta_test['ColorProfile/size'] = '3144'
    meta_test['channel_color_0'] = '255,0,0'
    meta_test['channel_color_1'] = '0,255,0'
    meta_test['channel_color_2'] = '0,0,255'
    meta_test['channel_color_3'] = '0,0,0'
    meta_test['channel_0_name'] = 'Red'
    meta_test['channel_1_name'] = 'Green'
    meta_test['channel_2_name'] = 'Blue'
    meta_test['channel_3_name'] = 'Alpha'
    test_metadata_read( "PNG RGBA", "IMG_0184_RGBA.png", meta_test )

    # reading metadata from OIB v 2.0.0.0
    meta_test = {}
    meta_test['date_time'] = '2010-06-04 08:26:06'
    meta_test['pixel_resolution_x'] = '0.206798'
    meta_test['pixel_resolution_y'] = '0.206798'
    meta_test['pixel_resolution_z'] = '0.428571'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['channel_0_name'] = 'Cy2'
    meta_test['channel_1_name'] = 'Cy3'
    meta_test['channel_2_name'] = 'Cy5'
    meta_test['display_channel_blue'] = '2'
    meta_test['display_channel_cyan'] = '-1'
    meta_test['display_channel_gray'] = '-1'
    meta_test['display_channel_green'] = '0'
    meta_test['display_channel_magenta'] = '-1'
    meta_test['display_channel_red'] = '1'
    meta_test['display_channel_yellow'] = '-1'
    test_metadata_read( "OIB.2", "CSR 4 mo COX g M cone r PNA b z1.oib", meta_test )

    # reading metadata from large OIB
    meta_test = {}
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '1024'
    meta_test['image_num_c'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '528'
    meta_test['pixel_resolution_x'] = '1.240787'
    meta_test['pixel_resolution_y'] = '1.240787'
    meta_test['pixel_resolution_z'] = '1.237652'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['date_time'] = '2012-01-25 15:35:02'
    meta_test['channel_0_name'] = 'None'
    test_metadata_read( "OIB.Large", "112811B_5.oib", meta_test )

    # reading metadata from OIB with strange Z planes and only actual one Z image
    meta_test = {}
    meta_test['image_num_c'] = '4'
    meta_test['image_num_p'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '640'
    meta_test['image_pixel_depth'] = '16'
    meta_test['date_time'] = '2010-11-19 14:32:33'
    meta_test['pixel_resolution_x'] = '0.496223'
    meta_test['pixel_resolution_y'] = '0.496223'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['channel_0_name'] = 'Alexa Fluor 405'
    meta_test['channel_1_name'] = 'Cy2'
    meta_test['channel_2_name'] = 'Cy3'
    meta_test['channel_3_name'] = 'Cy5'
    meta_test['display_channel_red']     = '2'
    meta_test['display_channel_green']   = '1'
    meta_test['display_channel_blue']    = '3'
    meta_test['display_channel_cyan']    = '-1'
    meta_test['display_channel_magenta'] = '-1'
    meta_test['display_channel_yellow']  = '-1'
    meta_test['display_channel_gray']    = '0'
    test_metadata_read( "OIB 2.0 One plane", "Retina 4 top.oib", meta_test )


    # reading metadata from Zeiss ZVI
    meta_test = {}
    meta_test['date_time'] = '2006-06-22 08:27:13'
    meta_test['pixel_resolution_x'] = '0.157153'
    meta_test['pixel_resolution_y'] = '0.157153'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['channel_0_name'] = 'Cy5'
    meta_test['channel_1_name'] = 'TRITC'
    meta_test['channel_2_name'] = 'FITC'
    meta_test['channel_3_name'] = 'DAPI'
    meta_test['objective/name'] = 'Plan Neofluar 40x/1.30 Oil Ph3 (DIC III) (440451)'
    meta_test['objective/magnification'] = 40.0
    meta_test['objective/numerical_aperture'] = 1.3
    test_metadata_read( "Zeiss ZVI", "23D3HA-cy3 psd-gfp-488 Homer-647 DIV 14 - 3.zvi", meta_test )

    # reading metadata from Zeiss ZVI
    meta_test = {}
    meta_test['image_num_c'] = '2'
    meta_test['image_num_p'] = '14'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '14'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_num_x'] = '1388'
    meta_test['image_num_y'] = '1040'
    meta_test['date_time'] = '2010-01-06 03:53:37'
    meta_test['pixel_resolution_x'] = '0.102381'
    meta_test['pixel_resolution_y'] = '0.102381'
    meta_test['pixel_resolution_z'] = '0.320000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['channel_0_name'] = 'DsRed'
    meta_test['channel_1_name'] = 'eGFP'
    meta_test['display_channel_red'] = '-1'
    meta_test['display_channel_green'] = '0'
    meta_test['display_channel_blue'] = '1'
    meta_test['display_channel_yellow'] = '-1'
    meta_test['display_channel_magenta'] = '-1'
    meta_test['display_channel_cyan'] = '-1'
    meta_test['display_channel_gray'] = '-1'
    meta_test['objective/name'] = 'C-Apochromat 63x/1.20 W Korr UV VIS IR'
    meta_test['objective/magnification'] = 63.0
    meta_test['objective/numerical_aperture'] = 1.2
    test_metadata_read( "Zeiss ZVI", "0022.zvi", meta_test )


    # reading metadata from Andor
    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_p'] = '12'
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['date_time'] = '2010-06-08 10:10:36'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['stage_position/0/x'] = '923.497006'
    meta_test['stage_position/0/y'] = '-1660.502994'
    meta_test['stage_position/0/z'] = '49.990000'
    test_metadata_read( "Andor", "MF Mon 2x2.tif", meta_test )


    # reading metadata from MicroManager OME-TIFF
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '3'
    meta_test['image_num_z'] = '3'
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['MicroManager/Objective-Label'] = 'Nikon 10X S Fluor'
    meta_test['MicroManager/Camera-CameraName'] = 'DemoCamera-MultiMode'
    test_metadata_read( "MicroManager OME-TIFF", "Untitled_MMImages_Pos0.ome.tif", meta_test )

    # reading metadata from DCRAW - Adobe DNG
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '2616'
    meta_test['image_num_y'] = '1960'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Image/Make'] = 'Canon'
    meta_test['Exif/Image/Model'] = 'Canon PowerShot G5'
    meta_test['Exif/Photo/ExposureTime'] = '1/160 s'
    meta_test['Exif/Photo/FNumber'] = 'F4'
    meta_test['Exif/Photo/FocalLength'] = '7.2 mm'
    meta_test['Exif/Photo/ISOSpeedRatings'] = '50'
    test_metadata_read( "Adobe DNG", "CRW_0136_COMPR.dng", meta_test )

    # reading metadata from DCRAW - Canon CR2
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '3522'
    meta_test['image_num_y'] = '2348'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Photo/ISOSpeedRatings'] = '100'
    meta_test['Exif/Photo/ExposureTime'] = '1/3 s'
    meta_test['Exif/Photo/FNumber'] = 'F5.6'
    test_metadata_read( "Canon CR2", "IMG_0040.CR2", meta_test )

    # reading metadata from DCRAW - Canon 5DSr
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '8736'
    meta_test['image_num_y'] = '5856'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['format'] = 'CANON-RAW'
    meta_test['image_mode'] = 'RGB'
    meta_test['Exif/Photo/Flash'] = 'No, compulsory'
    meta_test['Exif/Photo/FocalLength'] = '13.0 mm'
    meta_test['Exif/Photo/FNumber'] = 'F10'
    meta_test['custom/aperture'] = '10.000000'
    meta_test['custom/focal_length'] = '13.000000'
    meta_test['custom/iso_speed'] = '100.000000'
    meta_test['custom/make'] = 'Canon'
    meta_test['custom/model'] = 'EOS 5DS R'
    meta_test['custom/shutter'] = '0.005000'
    meta_test['date_time'] = '2016-05-24 15:16:35'
    meta_test['custom/aperture'] = '10.000000'
    meta_test['custom/aperture'] = '10.000000'
    meta_test['custom/aperture'] = '10.000000'
    test_metadata_read( "Canon 5DSr CR2", "6J0A3548.CR2", meta_test )

    # reading metadata from DCRAW - Canon CRW
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '2616'
    meta_test['image_num_y'] = '1960'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Image/Make'] = 'Canon'
    meta_test['Exif/Image/Model'] = 'Canon PowerShot G5'
    meta_test['Exif/Photo/ExposureTime'] = '1/156 s'
    meta_test['Exif/Photo/FNumber'] = 'F4'
    test_metadata_read( "Canon CRW", "CRW_0136.CRW", meta_test )

    # reading metadata from DCRAW - Pentax
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '3008'
    meta_test['image_num_y'] = '2000'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '8'
    meta_test['Exif/Photo/ExposureTime'] = '1/125 s'
    meta_test['Exif/Photo/FNumber'] = 'F8'
    meta_test['Exif/Photo/ISOSpeedRatings'] = '200'
    test_metadata_read( "Pentax", "PENTAX_IMGP1618.JPG", meta_test )

    # reading metadata from DCRAW - Minolta
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '3016'
    meta_test['image_num_y'] = '2008'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Photo/ExposureTime'] = '1/80 s'
    meta_test['Exif/Photo/FNumber'] = 'F20'
    meta_test['Exif/Photo/ISOSpeedRatings'] = '100'
    test_metadata_read( "Minolta", "PICT1694.MRW", meta_test )


    # reading metadata from DCRAW - Nikon
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '2576'
    meta_test['image_num_y'] = '1924'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Photo/ExposureTime'] = '1 s'
    meta_test['Exif/Photo/FNumber'] = 'F4.2'
    test_metadata_read( "Nikon", "DSCN0041.NEF", meta_test )

    # reading metadata from DCRAW - Olympus
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '3088'
    meta_test['image_num_y'] = '2310'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Photo/ExposureTime'] = '1/30 s'
    meta_test['Exif/Photo/FNumber'] = 'F2.8'
    meta_test['Exif/Photo/ISOSpeedRatings'] = '125'
    test_metadata_read( "Olympus", "P1110010.ORF", meta_test )

    # GeoTIFF
    meta_test = {}
    meta_test['image_num_x'] = '1281'
    meta_test['image_num_y'] = '1037'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '8'
    meta_test['Geo/Tags/ModelPixelScaleTag'] = '0.179859484777536,0.179826422372659,0'
    meta_test['Geo/Tags/ModelTiepointTag'] = '0,0,0;719865.328538339,9859359.12604843,0'
    meta_test['Geo/Model/projection'] = '16136 (UTM zone 36S)'
    meta_test['Geo/Model/proj4_definition'] = '+proj=tmerc +lat_0=0.000000000 +lon_0=33.000000000 +k=0.999600 +x_0=500000.000 +y_0=10000000.000 +ellps=WGS84 +units=m'
    meta_test['Geo/Coordinates/center'] = '-1.2725008,34.9769992'
    meta_test['Geo/Coordinates/center_model'] = '719980.529,9859265.886'
    meta_test['Geo/Coordinates/upper_left'] = '-1.2716586,34.9759636'
    meta_test['Geo/Coordinates/upper_left_model'] = '719865.329,9859359.126'
    test_metadata_read( "GeoTIFF", "test4.tif", meta_test )


    meta_test = {}
    meta_test['image_num_x'] = '1200'
    meta_test['image_num_y'] = '1650'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_metadata_read( "JPEG-XR", "219j_q050.jxr", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '1200'
    meta_test['image_num_y'] = '1650'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_metadata_read( "JPEG-2000", "219j.jp2", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '1200'
    meta_test['image_num_y'] = '1650'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_metadata_read( "WEBP", "219j_q080.webp", meta_test )

    # DICOMs

    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'signed integer'
    meta_test['pixel_resolution_x'] = '0.439453'
    meta_test['pixel_resolution_y'] = '0.439453'
    meta_test['pixel_resolution_z'] = '5.000000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['DICOM/Rescale Intercept (0028,1052)'] = '-1024'
    meta_test['DICOM/Rescale Slope (0028,1053)'] = '1'
    meta_test['DICOM/Rescale Type (0028,1054)'] = 'HU'
    meta_test['DICOM/Scan Options (0018,0022)'] = 'AXIAL MODE'
    meta_test['DICOM/Series Description (0008,103e)'] = 'HEAD ST W/O'
    meta_test['DICOM/Window Center (0028,1050)'] = '40'
    meta_test['DICOM/Window Width (0028,1051)'] = '80'
    meta_test['DICOM/Patient\'s Age (0010,1010)'] = '035Y'
    test_metadata_read( "DICOM CT", "10", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '16'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_z'] = '10.00'
    meta_test['pixel_resolution_t'] = '69.470000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['pixel_resolution_unit_t'] = 'seconds'
    meta_test['DICOM/Modality (0008,0060)'] = 'MR'
    meta_test['DICOM/Study Description (0008,1030)'] = 'MRI'
    test_metadata_read( "DICOM MR", "MR-MONO2-8-16x-heart", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '128'
    meta_test['image_num_y'] = '120'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '8'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '4.000000'
    meta_test['pixel_resolution_y'] = '3.000000'
    meta_test['pixel_resolution_t'] = '100.000000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_t'] = 'seconds'
    meta_test['DICOM/Modality (0008,0060)'] = 'US'
    meta_test['DICOM/Study Description (0008,1030)'] = 'Exercise Echocardiogram'
    test_metadata_read( "DICOM EXECHO", "US-MONO2-8-8x-execho", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '600'
    meta_test['image_num_y'] = '430'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '10'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_t'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_t'] = 'seconds'
    meta_test['DICOM/Modality (0008,0060)'] = 'US'
    meta_test['DICOM/Study Description (0008,1030)'] = 'Echocardiogram'
    test_metadata_read( "DICOM ECHO", "US-PAL-8-10x-echo", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '1024'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['DICOM/Modality (0008,0060)'] = 'RF'
    meta_test['DICOM/Patient\'s Sex (0010,0040)'] = 'F'
    test_metadata_read( "DICOM RF", "0015.DCM", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '600'
    meta_test['image_num_y'] = '430'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '11'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_t'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_t'] = 'seconds'
    meta_test['DICOM/Modality (0008,0060)'] = 'US'
    meta_test['DICOM/Patient\'s Sex (0010,0040)'] = 'F'
    meta_test['DICOM/Study Description (0008,1030)'] = 'Echocardiogram'
    test_metadata_read( "DICOM US", "0020.DCM", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'signed integer'
    meta_test['pixel_resolution_x'] = '1.015620'
    meta_test['pixel_resolution_y'] = '1.015620'
    meta_test['pixel_resolution_z'] = '5'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['DICOM/Modality (0008,0060)'] = 'MR'
    meta_test['DICOM/Patient\'s Sex (0010,0040)'] = 'M'
    meta_test['DICOM/Study Description (0008,1030)'] = 'ADNI RESEARCH STUDY'
    meta_test['DICOM/Slice Location (0020,1041)'] = '-46.90000153'
    test_metadata_read( "DICOM ADNI", "ADNI_002_S_0295_MR_3-plane_localizer__br_raw_20060418193538653_1_S13402_I13712.dcm", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '1.093750'
    meta_test['pixel_resolution_y'] = '1.093750'
    meta_test['pixel_resolution_z'] = '10'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['DICOM/Modality (0008,0060)'] = 'MR'
    meta_test['DICOM/Patient\'s Sex (0010,0040)'] = 'F'
    meta_test['DICOM/Study Description (0008,1030)'] = 'RaiGoe^standaard'
    meta_test['DICOM/Slice Location (0020,1041)'] = '0'
    test_metadata_read( "DICOM MR", "BetSog_20040312_Goebel_C2-0001-0001-0001.dcm", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'DICOM'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '0.361328'
    meta_test['pixel_resolution_y'] = '0.361328'
    meta_test['pixel_resolution_z'] = '0.75'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['DICOM/Modality (0008,0060)'] = 'CT'
    meta_test['DICOM/Window Center (0028,1050)'] = '400\\\\700'
    meta_test['DICOM/Window Width (0028,1051)'] = '2000\\\\4000'
    test_metadata_read( "DICOM CT", "IM-0001-0001.dcm", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '64'
    meta_test['image_num_y'] = '64'
    meta_test['image_num_z'] = '21'
    meta_test['image_num_t'] = '180'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'NIFTI'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'signed integer'
    meta_test['pixel_resolution_x'] = '4.000000'
    meta_test['pixel_resolution_y'] = '4.000000'
    meta_test['pixel_resolution_z'] = '6.000000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    test_metadata_read( "NIFTI functional", "filtered_func_data.nii", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_z'] = '32'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'NIFTI'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '0.439453'
    meta_test['pixel_resolution_y'] = '0.439453'
    meta_test['pixel_resolution_z'] = '5.000000'
    #meta_test['pixel_resolution_unit_x'] = 'mm' # not stored in the image
    #meta_test['pixel_resolution_unit_y'] = 'mm'
    #meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['NIFTI/affine_transform'] = '0.439453,0.000000,0.000000,-112.060516;0.000000,0.439453,0.000000,-112.060516;0.000000,0.000000,5.000000,-75.000000'
    test_metadata_read( "NIFTI transforms", "16_day1_1_patient_29C93FK6_1.nii", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '64'
    meta_test['image_num_y'] = '64'
    meta_test['image_num_z'] = '35'
    meta_test['image_num_t'] = '147'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'NIFTI'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'signed integer'
    meta_test['pixel_resolution_x'] = '3.437500'
    meta_test['pixel_resolution_y'] = '3.437500'
    meta_test['pixel_resolution_z'] = '3.999421'
    #meta_test['pixel_resolution_unit_x'] = 'mm' # not stored in the image
    #meta_test['pixel_resolution_unit_y'] = 'mm'
    #meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['pixel_resolution_unit_t'] = 'ms'
    meta_test['NIFTI/quaternion'] = '0.000000,0.025615,0.999672;108.281250,103.739151,-83.445091'
    meta_test['XCEDE/study/series/acquisition_protocol/parameters/receivecoilname'] = 'HEAD'
    meta_test['XCEDE/study/series/id'] = '1'
    meta_test['XCEDE/subject/id'] = '103'
    meta_test['XCEDE/study/series/scanner/manufacturer'] = 'GE'
    meta_test['XCEDE/study/series/scanner/model'] = 'LX NVi 4T'
    test_metadata_read( "NIFTI XCEDE", "newsirp_final_XML.nii", meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '260'
    meta_test['image_num_y'] = '311'
    meta_test['image_num_z'] = '260'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['format']      = 'NIFTI'
    meta_test['image_pixel_depth'] = '32'
    meta_test['image_pixel_format'] = 'floating point'
    meta_test['pixel_resolution_x'] = '0.700000'
    meta_test['pixel_resolution_y'] = '0.700000'
    meta_test['pixel_resolution_z'] = '0.700000'
    meta_test['pixel_resolution_unit_x'] = 'mm'
    meta_test['pixel_resolution_unit_y'] = 'mm'
    meta_test['pixel_resolution_unit_z'] = 'mm'
    meta_test['NIFTI/affine_transform'] = '-0.700000,0.000000,0.000000,90.000000;0.000000,0.700000,0.000000,-126.000000;0.000000,0.000000,0.700000,-72.000000'
    test_metadata_read( "NIFTI GZ", "T1w.nii.gz", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5472'
    meta_test['image_num_y'] = '3648'
    meta_test['image_num_z'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['Exif/Photo/ExposureTime'] = '1/400 s'
    meta_test['Exif/Photo/FNumber'] = 'F11'
    meta_test['Exif/Photo/ISOSpeedRatings'] = '100'
    meta_test['Iptc/Application2/ProvinceState'] = 'California'
    meta_test['Exif/GPSInfo/GPSLatitude'] = '34deg 29.12550\''
    test_metadata_read( "JPEG-XR", "IMG_1913_16bit_prophoto_q90.jxr", meta_test )

    # JPEG-2000 and WebP metadata are not yet implemented
#    meta_test = {}
#    meta_test['image_num_c'] = '3'
#    meta_test['image_num_t'] = '1'
#    meta_test['image_num_z'] = '1'
#    meta_test['image_num_x'] = '5472'
#    meta_test['image_num_y'] = '3648'
#    meta_test['image_num_z'] = '1'
#    meta_test['image_pixel_depth'] = '16'
#    meta_test['Exif/Photo/ExposureTime'] = '1/400 s'
#    meta_test['Exif/Photo/FNumber'] = 'F11'
#    meta_test['Exif/Photo/ISOSpeedRatings'] = '100'
#    meta_test['Iptc/Application2/ProvinceState'] = 'California'
#    meta_test['Exif/GPSInfo/GPSLatitude'] = '34deg 29.12550'
#    test_metadata_read( "JPEG-2000", "IMG_1913_16bit_prophoto_ts1024_q90.jp2", meta_test )
#
#    meta_test = {}
#    meta_test['image_num_c'] = '3'
#    meta_test['image_num_t'] = '1'
#    meta_test['image_num_z'] = '1'
#    meta_test['image_num_x'] = '5472'
#    meta_test['image_num_y'] = '3648'
#    meta_test['image_num_z'] = '1'
#    meta_test['image_pixel_depth'] = '16'
#    meta_test['Exif/Photo/ExposureTime'] = '1/400 s'
#    meta_test['Exif/Photo/FNumber'] = 'F11'
#    meta_test['Exif/Photo/ISOSpeedRatings'] = '100'
#    meta_test['Iptc/Application2/ProvinceState'] = 'California'
#    meta_test['Exif/GPSInfo/GPSLatitude'] = '34deg 29.12550'
#    test_metadata_read( "WebP", "IMG_1913_prophoto_q90.webp", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5120'
    meta_test['image_num_y'] = '3840'
    meta_test['image_pixel_depth'] = '32'
    meta_test['image_pixel_format'] = 'floating point'
    meta_test['pixel_resolution_x'] = '2.510000'
    meta_test['pixel_resolution_y'] = '2.510000'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'angstroms'
    meta_test['MRC/alpha'] = '90.000000'
    test_metadata_read( "MRC", "20150917_05195_DNA-TET-25k-DE20_raw.region_000.sum-all_003-072.mrc", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_t'] = '32'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'signed integer'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'angstroms'
    meta_test['MRC/alpha'] = '90.000000'
    test_metadata_read( "MRC", "golgi.mrc", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '2048'
    meta_test['image_num_y'] = '2048'
    meta_test['image_pixel_depth'] = '16'
    meta_test['image_pixel_format'] = 'unsigned integer'
    meta_test['pixel_resolution_x'] = '0.000000'
    meta_test['pixel_resolution_y'] = '0.000000'
    meta_test['pixel_resolution_z'] = '0.000000'
    meta_test['pixel_resolution_unit_x'] = 'angstroms'
    meta_test['MRC/alpha'] = '0.000000'
    meta_test['FEI/magnification'] = '84.000000'
    meta_test['FEI/a_tilt'] = '-0.000066'
    test_metadata_read( "MRC", "Tile_19491580_0_1.mrc", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_t'] = '78'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '572'
    meta_test['image_num_y'] = '378'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'signed integer'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_z'] = '1.000000'
    meta_test['pixel_resolution_unit_x'] = 'angstroms'
    meta_test['MRC/alpha'] = '90.000000'
    test_metadata_read( "MRC", "dual.rec", meta_test )

if 'all' in mode or 'video' in mode:
    print
    print
    print '***************************************************'
    print '"video" - Reading video meta'
    print '***************************************************'

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '101'
    meta_test['image_num_p'] = '101'
    meta_test['image_num_x'] = '720'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970000'
    meta_test['video_codec_name'] = 'cinepak'
    test_image_video( "AVI CINEPAK", "122906_3Ax(inverted).avi", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '37'
    meta_test['image_num_p'] = '37'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '20.000000'
    meta_test['video_codec_name'] = 'cinepak'
    test_image_video( "AVI CINEPAK", "241aligned.avi", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '199'
    meta_test['image_num_p'] = '199'
    meta_test['image_num_x'] = '826'
    meta_test['image_num_y'] = '728'
    meta_test['video_frames_per_second'] = '30.000000'
    meta_test['video_codec_name'] = 'mpeg4'
    test_image_video( "QuickTime MPEG4", "3Dstack.tif.3D.mov", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '73'
    meta_test['image_num_p'] = '73'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '947'
    meta_test['video_frames_per_second'] = '30.303030'
    meta_test['video_codec_name'] = 'rawvideo'
    test_image_video( "AVI RAW", "B4nf.RS.RE.z1.5.15.06.3DAnimation.avi", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '2879' # 2878
    meta_test['image_num_p'] = '2879' # 2878
    meta_test['image_num_x'] = '352'
    meta_test['image_num_y'] = '240'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'mpeg1video'
    test_image_video( "MPEG", "EleanorRigby.mpg", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '301'
    meta_test['image_num_p'] = '301'
    meta_test['image_num_x'] = '884'
    meta_test['image_num_y'] = '845'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'mpeg4'
    test_image_video( "QuickTime MPEG4", "Muller cell z4.oib.3D.mov", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '9'
    meta_test['image_num_p'] = '9'
    meta_test['image_num_x'] = '316'
    meta_test['image_num_y'] = '400'
    meta_test['video_frames_per_second'] = '9.000009'
    meta_test['video_codec_name'] = 'rawvideo'
    test_image_video( "AVI RAW", "radiolaria.avi", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '2773'
    meta_test['image_num_p'] = '2773'
    meta_test['image_num_x'] = '1440'
    meta_test['image_num_y'] = '1080'
    meta_test['video_frames_per_second'] = '24.000000'
    meta_test['video_codec_name'] = 'wmv3'
    test_image_video( "WMV", "Step_into_Liquid_1080.wmv", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '199'
    meta_test['image_num_p'] = '199'
    meta_test['image_num_x'] = '826'
    meta_test['image_num_y'] = '728'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'theora'
    test_image_video( "OGG", "test.ogv", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '36777' # 36776
    meta_test['image_num_p'] = '36777' # 36776
    meta_test['image_num_x'] = '1440'
    meta_test['image_num_y'] = '1080'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'mpeg2video'
    test_image_video( "MPEG2 TS (1)", "B01C0201.M2T", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '8562' # 17127
    meta_test['image_num_p'] = '8562' # 17127
    meta_test['image_num_x'] = '1440'
    meta_test['image_num_y'] = '1080'
    meta_test['video_frames_per_second'] = '29.970030' #'59.940060'
    meta_test['video_codec_name'] = 'h264'
    test_image_video( "MPEG2 TS (2)", "Girsh_path3.m2ts", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '28'
    meta_test['image_num_p'] = '28'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'mpeg4'
    test_image_video( "AVI MPEG4", "out.avi", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '30'
    meta_test['image_num_p'] = '30'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'flv'
    test_image_video( "Flash video", "out.flv", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '28'
    meta_test['image_num_p'] = '28'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'mpeg4'
    test_image_video( "MPEG4 H.263", "out.m4v", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '30'
    meta_test['image_num_p'] = '30'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'h264'
    test_image_video( "Matroska MPEG4 H.264", "out.mkv", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '28'
    meta_test['image_num_p'] = '28'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'h264'
    test_image_video( "Quicktime MPEG4 H.264", "out.mov", meta_test )

#    meta_test = {}
#    meta_test['image_num_z'] = '1'
#    meta_test['image_num_t'] = '28'
#    meta_test['image_num_p'] = '28'
#    meta_test['image_num_x'] = '640'
#    meta_test['image_num_y'] = '480'
#    meta_test['video_frames_per_second'] = '29.970030'
#    meta_test['video_codec_name'] = 'mpeg2video'
#    test_image_video( "MPEG2", "out.mpg", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '31'
    meta_test['image_num_p'] = '31'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'theora'
    test_image_video( "OGG Theora", "out.ogg", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '31' # 30
    meta_test['image_num_p'] = '31' # 30
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'mpeg1video'
    test_image_video( "MPEG1", "out.vcd", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '27'
    meta_test['image_num_p'] = '27'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'msmpeg4'
    test_image_video( "WMV", "out.wmv", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '30'
    meta_test['image_num_p'] = '30'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'vp8'
    test_image_video( "WebM", "out.webm", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '28'
    meta_test['image_num_p'] = '28'
    meta_test['image_num_x'] = '640'
    meta_test['image_num_y'] = '480'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'h264'
    test_image_video( "MPEG4 AVC H.264", "out_h264.mp4", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '25'
    meta_test['image_num_p'] = '25'
    meta_test['image_num_x'] = '1920'
    meta_test['image_num_y'] = '1080'
    meta_test['image_num_c'] = '3'
    meta_test['video_frames_per_second'] = '29.970030'
    meta_test['video_codec_name'] = 'hevc'
    test_image_video( "MPEG4 AVC H.265", "out_h265.mp4", meta_test )

if 'all' in mode or 'transforms' in mode:
    print
    print
    print '***************************************************'
    print '"transforms" - Computing transforms'
    print '***************************************************'

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    test_image_transforms( '-filter', 'edge', "flowers_24bit_nointr.png", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    test_image_transforms( '-filter', 'wndchrmcolor', "flowers_24bit_nointr.png", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    test_image_transforms( '-transform_color', 'rgb2hsv', "flowers_24bit_nointr.png", meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_pixel_format'] = 'unsigned integer'
    test_image_transforms( '-transform_color', 'hsv2rgb', 'flowers_24bit_hsv.tif', meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '768'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '64'
    meta_test['image_pixel_format'] = 'floating point'
    test_image_transforms( '-transform', 'chebyshev', 'flowers_8bit_gray.png', meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '64'
    meta_test['image_pixel_format'] = 'floating point'
    test_image_transforms( '-transform', 'fft', 'flowers_8bit_gray.png', meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '1032'
    meta_test['image_num_y'] = '776'
    meta_test['image_pixel_depth'] = '64'
    meta_test['image_pixel_format'] = 'floating point'
    test_image_transforms( '-transform', 'wavelet', 'flowers_8bit_gray.png', meta_test )

    meta_test = {}
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '180'
    meta_test['image_num_y'] = '1283'
    meta_test['image_pixel_depth'] = '64'
    meta_test['image_pixel_format'] = 'floating point'
    test_image_transforms( '-transform', 'radon', 'flowers_8bit_gray.png', meta_test )

if 'all' in mode or 'commands' in mode:
    print
    print
    print '***************************************************'
    print '"commands" - Computing commands'
    print '***************************************************'

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_x'] = '3264'
    meta_test['image_num_y'] = '2448'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-rotate', 'guess'], "IMG_0562.JPG", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_x'] = '2448'
    meta_test['image_num_y'] = '3264'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-rotate', 'guess'], "IMG_0593.JPG", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_x'] = '3264'
    meta_test['image_num_y'] = '2448'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-rotate', 'guess'], "IMG_1003.JPG", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '32'
    test_image_commands( ['-superpixels', '16'], 'flowers_8bit_gray.png', meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '1'
    meta_test['image_num_x'] = '1024'
    meta_test['image_num_y'] = '768'
    meta_test['image_pixel_depth'] = '32'
    test_image_commands( ['-superpixels', '16'], 'flowers_24bit_nointr.png', meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '2'
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_z'] = '7'
    meta_test['image_pixel_depth'] = '16'
    meta_test['pixel_resolution_x'] = '0.414320'
    meta_test['pixel_resolution_y'] = '0.414320'
    meta_test['pixel_resolution_z'] = '1.857143'
    meta_test['channel_0_name'] = 'FITC'
    meta_test['channel_1_name'] = 'Cy3'
    test_image_commands( ['-t', 'ome-tiff', '-resize3d', '256,0,0,TC'], '161pkcvampz1Live2-17-2004_11-57-21_AM.tif', meta_test )

    meta_test = {}
    meta_test['image_num_p'] = '256'
    meta_test['image_num_c'] = '2'
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '13'
    meta_test['image_num_z'] = '256'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['pixel_resolution_x'] = '0.207160'
    meta_test['pixel_resolution_y'] = '1.000000'
    meta_test['pixel_resolution_z'] = '0.414320'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['pixel_resolution_unit_t'] = 'seconds'
    meta_test['channel_0_name'] = 'FITC'
    meta_test['channel_1_name'] = 'Cy3'
    test_image_commands( ['-t', 'ome-tiff', '-rearrange3d', 'xzy'], 'cells.ome.tif', meta_test )

    meta_test = {}
    meta_test['image_num_p'] = '512'
    meta_test['image_num_c'] = '2'
    meta_test['image_num_x'] = '13'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_z'] = '512'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '16'
    meta_test['pixel_resolution_x'] = '1.000000'
    meta_test['pixel_resolution_y'] = '0.414320'
    meta_test['pixel_resolution_z'] = '0.207160'
    meta_test['pixel_resolution_unit_x'] = 'microns'
    meta_test['pixel_resolution_unit_y'] = 'microns'
    meta_test['pixel_resolution_unit_z'] = 'microns'
    meta_test['pixel_resolution_unit_t'] = 'seconds'
    meta_test['channel_0_name'] = 'FITC'
    meta_test['channel_1_name'] = 'Cy3'
    test_image_commands( ['-t', 'ome-tiff', '-rearrange3d', 'yzx'], 'cells.ome.tif', meta_test )


if 'all' in mode or 'pyramids' in mode:
    print
    print
    print '***************************************************'
    print '"pyramids" - Testing pyramidal images'
    print '***************************************************'

    # test reading pyramidal metadata
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5000'
    meta_test['image_num_y'] = '2853'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '4'
    meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000'
    test_metadata_read( "Photoshop pyramid", "monument_photoshop.tif", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5000'
    meta_test['image_num_y'] = '2853'
    meta_test['tile_num_x'] = '512'
    meta_test['tile_num_y'] = '512'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '5'
    meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062400'
    test_metadata_read( "Imagemagick pyramid", "monument_vips.tif", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5000'
    meta_test['image_num_y'] = '2853'
    meta_test['tile_num_x'] = '512'
    meta_test['tile_num_y'] = '512'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '7'
    meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062400,0.031200,0.015600'
    test_metadata_read( "Top-IFD TIFF pyramid", "monument_imgcnv_topdirs.tif", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5000'
    meta_test['image_num_y'] = '2853'
    meta_test['tile_num_x'] = '512'
    meta_test['tile_num_y'] = '512'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '7'
    meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062400,0.031200,0.015600'
    test_metadata_read( "Sub-IFD TIFF pyramid", "monument_imgcnv_subdirs.tif", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5000'
    meta_test['image_num_y'] = '2853'
    meta_test['tile_num_x'] = '512'
    meta_test['tile_num_y'] = '512'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '7'
    meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062400,0.031200,0.015600'
    test_metadata_read( "OME-TIFF pyramid", "monument_imgcnv.ome.tif", meta_test )

    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '5000'
    meta_test['image_num_y'] = '2853'
    meta_test['tile_num_x'] = '256'
    meta_test['tile_num_y'] = '256'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '7'
    meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062400,0.031200,0.015600'
    test_metadata_read( "256px tiles TIFF pyramid", "monument_imgcnv.256.tif", meta_test )

    # test reading pyramidal levels
    meta_test = {}
    meta_test['image_num_x'] = '156'
    meta_test['image_num_y'] = '89'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '5'], 'monument_imgcnv_subdirs.tif', meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '156'
    meta_test['image_num_y'] = '89'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '5'], 'monument_imgcnv_topdirs.tif', meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '156'
    meta_test['image_num_y'] = '89'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '5'], 'monument_imgcnv.ome.tif', meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '156'
    meta_test['image_num_y'] = '89'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '5'], 'monument_imgcnv.256.tif', meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '312'
    meta_test['image_num_y'] = '178'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '4'], 'monument_vips.tif', meta_test )

    meta_test = {}
    meta_test['image_num_x'] = '625'
    meta_test['image_num_y'] = '357'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '3'], 'monument_photoshop.tif', meta_test )

    # test reading pyramidal tiles

    # - subifds
    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,0,0,2'], 'monument_imgcnv_subdirs.tif', meta_test ) # first tile

    meta_test = {}
    meta_test['image_num_x'] = '226'
    meta_test['image_num_y'] = '201'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,2,1,2'], 'monument_imgcnv_subdirs.tif', meta_test ) # last tile

    meta_test = {}
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '256,1,1,2'], 'monument_imgcnv_subdirs.tif', meta_test ) # tile size different from stored

    # - vips
    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,0,0,2'], 'monument_vips.tif', meta_test ) # first tile

    meta_test = {}
    meta_test['image_num_x'] = '226'
    meta_test['image_num_y'] = '201'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,2,1,2'], 'monument_vips.tif', meta_test ) # last tile

    meta_test = {}
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '256,1,1,2'], 'monument_vips.tif', meta_test ) # tile size different from stored

    # - ome-tiff
    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,0,0,2'], 'monument_imgcnv.ome.tif', meta_test ) # first tile

    meta_test = {}
    meta_test['image_num_x'] = '226'
    meta_test['image_num_y'] = '201'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,2,1,2'], 'monument_imgcnv.ome.tif', meta_test ) # last tile

    meta_test = {}
    meta_test['image_num_x'] = '256'
    meta_test['image_num_y'] = '256'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '256,1,1,2'], 'monument_imgcnv.ome.tif', meta_test ) # tile size different from stored

    # JPEG-2000

    # meta
    meta_test = {}
    meta_test['image_num_c'] = '3'
    meta_test['image_num_t'] = '1'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_x'] = '17334'
    meta_test['image_num_y'] = '17457'
    meta_test['tile_num_x'] = '2048'
    meta_test['tile_num_y'] = '2048'
    meta_test['image_pixel_depth'] = '8'
    meta_test['image_num_resolution_levels'] = '8'
    if os.name == 'nt': #dima: some minor precision differences, to be checked
        meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062500,0.031250,0.015625,0.007813'
    else:
        meta_test['image_resolution_level_scales'] = '1.000000,0.500000,0.250000,0.125000,0.062500,0.031250,0.015625,0.007812'
    meta_test['image_resolution_level_structure'] = 'flat'
    test_metadata_read( "2048px tiles JPEG-2000 pyramid", "retina.jp2", meta_test )

    # levels
    meta_test = {}
    meta_test['image_num_x'] = '271'
    meta_test['image_num_y'] = '273'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-res-level', '6'], 'retina.jp2', meta_test )

    # tiles
    meta_test = {}
    meta_test['image_num_x'] = '512'
    meta_test['image_num_y'] = '512'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,0,0,4'], 'retina.jp2', meta_test ) # first tile

    meta_test = {}
    meta_test['image_num_x'] = '59'
    meta_test['image_num_y'] = '67'
    meta_test['image_num_c'] = '3'
    meta_test['image_num_z'] = '1'
    meta_test['image_num_t'] = '1'
    meta_test['image_pixel_depth'] = '8'
    test_image_commands( ['-t', 'tiff', '-tile', '512,2,2,4'], 'retina.jp2', meta_test ) # last tile

    # testing tile size different from stored is not required for flat structure


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
