#!/usr/bin/env python

import sys, os
from subprocess import *

# =================================================================
systype = os.getenv("SYS_TYPE")
testdir = os.path.abspath(os.path.dirname(sys.argv[0]))

if not systype:
    systype = ''

# =================================================================
def errexit (msg):
    print '*********************************************************************'
    print "Error: " + msg
    print '*********************************************************************'
    sys.exit(1)

# =================================================================
def FindBinDir(progname):
    # try ../$SYS_TYPE/bin (usual case) and ../../$SYS_TYPE/bin (dev on LC)
    for dots in ['..','../..','../../..']:
        for subdir in [systype, '.']:
            trydir = "%s/%s/%s/bin"%(testdir,dots,subdir)
            # sys.stderr.write( "trying directory: %s\n"%trydir)
            if  os.path.exists(trydir+'/' + progname):
                return trydir
    return "%s/../%s/bin"%(testdir,systype)

# =================================================================
def CheckAndFixDir(theDir):
    if not os.path.exists(theDir):
        return None
    theDir = os.path.realpath(theDir)
    # fix relative paths and stuff:
    if theDir[-1] != '/':
        theDir = theDir + "/" 
    if theDir[0] != '/':
        theDir = os.getcwd() + '/' + theDir

    return theDir


# =================================================================
def FindBinary(bindir, binary):    
    if bindir:
        binary = "%s/%s"%(bindir,binary)   
    try:
        p = Popen("which %s"%binary, shell=True, stdout=PIPE, stderr=STDOUT)
        p.wait()
        binary = p.stdout.read().strip()
    except:
        sys.stderr.write( "Error: 'which' could not find binary %s\n"%binary)        
        raise
    
    if not os.path.exists(binary):
        errexit( "Error: os.path() could not find binary %s"%binary)

    return binary

