#!/usr/bin/python


""" imgcnv building framework
"""

__author__    = "Dmitry Fedorov"
__version__   = "1.6"
__copyright__ = "Center for BioImage Informatics, University California, Santa Barbara"

import sys
import os
from copy import deepcopy
from subprocess import Popen, call, PIPE
import time
import urllib
import posixpath
import tarfile
import zipfile

url_libs_store = 'http://bitbucket.org/dimin/bioimageconvert/downloads/'
local_store_libs  = 'libs'
lib_version = '2-0-9'

if os.name == 'nt':
    sys_libs_dif = 'vc2013_x64'
elif sys.platform == 'darwin':
    sys_libs_dif = 'macosx'
else:
    sys_libs_dif = 'linux'

###############################################################
# misc
###############################################################

def fetch_file(filename):
    url = posixpath.join(url_libs_store, filename)
    path = os.path.join(local_store_libs, filename)
    if not os.path.exists(path):
        urllib.urlretrieve(url, path)
    if not os.path.exists(path):
        print '!!! Could not find required package: "%s" !!!'%filename
    return path

def unZip(filename, foldername):
    z = zipfile.ZipFile(filename, 'r')

    # first test if archive is valid
    names = z.namelist()
    for name in names:
        if name.startswith('/') or name.startswith('\\') or name.startswith('..'):
            z.close()
            return []

    # extract members if all is fine
    z.extractall(foldername)
    z.close()
    return names

# unpacking that preserves structure
def unTar(filename, foldername):
    z = tarfile.open(filename, 'r')

    # first test if archive is valid
    names = z.getnames()
    for name in names:
        if name.startswith('/') or name.startswith('\\') or name.startswith('..'):
            z.close()
            return []

    # extract members if all is fine
    z.extractall(foldername)
    z.close()
    return names

def unPack(filename, folderName):
    try:
        return unZip(filename, folderName)
    except zipfile.BadZipfile:
        return unTar(filename, folderName)

###############################################################
# run tests
###############################################################

try:
    os.mkdir(os.path.join(local_store_libs, sys_libs_dif))
except:
    pass

#**************************************************************
# download package
#**************************************************************

filename = 'build_libs_%s_%s.zip'%(lib_version, sys_libs_dif)
print 'Downloading %s'%filename
fetch_file(filename)

print 'Unpacking to %s'%(os.path.join(local_store_libs, sys_libs_dif))
unPack(os.path.join(local_store_libs, filename), local_store_libs)

