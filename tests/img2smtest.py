#!/usr/bin/env python

import sys, os, shutil, time, threading
from subprocess import *

def errexit (msg):
    print msg
    sys.exit(1)

def usage():
    print "Usage:  img2smtest.py [options]"
    print "Options:"
    print " -bindir dirnam:  path to the directory containing sm2img for disambiguation -- typically this will be $INSTALL_DIR/bin"

bindir=None
# PARSE ARGUMENTS:
if "-bindir" in sys.argv:
    pos = sys.argv.index('-bindir')
    if len(sys.argv) < pos+2:
        usage()
        sys.exit(-1)
    bindir=sys.argv[pos+1]

# SET UP BINDIR
systype = os.getenv("SYS_TYPE")
   
if not bindir:
    # try ../$SYS_TYPE/bin (usual case) and ../../$SYS_TYPE/bin (dev on LC)
    for dots in ['..','../..']:
        trydir = 'os.path.abspath(sys.argv[0])/%s/%s/bin'%(dots,systype)
        if  os.path.exists(trydir+'/sm2img'):
            bindir=trydir
            break
    
# if user chose bindir, then fix relative paths and stuff:
if bindir:
    if bindir[-1] != '/':
        bindir = bindir + "/" 
    if bindir[0] != '/':
        bindir = os.getcwd() + '/' + bindir


# FIND IMG2SM
img2sm = "img2sm"
if bindir:
    img2sm = bindir + 'img2sm'
    
try:
    p = Popen("which %s"%img2sm, shell=True, stdout=PIPE, stderr=STDOUT)
    p.wait()
    img2sm = p.stdout.read().strip()
except:
    print "Error:  could not find img2sm in PATH or bindir"
    raise

bindir = os.path.abspath(os.path.dirname(img2sm))
sys.stderr.write( "bindir is: "+bindir)
sys.stderr.write( "Found img2sm at %s"% img2sm)

# FIND DATA DIR
datadir = os.path.abspath(os.path.dirname(sys.argv[0])+'/../sample-data')
sys.stderr.write( "datadir is "+ datadir)

# RUN TESTS:
testdir = "/tmp/"+os.getenv("USER")+"/img2smtest/"
shutil.rmtree(testdir, ignore_errors=True) 
os.makedirs(testdir)
if not os.path.exists(testdir):
    errexit("Cannot create test output directory "+testdir)

proc = None
    
def runcmd(cmd,outfile):
    global proc
    cmd="%s %s %s"%(img2sm, cmd,outfile) 
    sys.stderr.write( "Running test: \"%s\\n"%cmd)
    sys.stderr.write("lengh of command: %d\n"%len(cmd))
    proc = Popen(cmd.split(), bufsize=-1)
    proc.wait()
    return
    
def testrun(cmd,outfile, timeout=5):
    global success, errmsg, proc
    theThread = threading.Thread(target=runcmd,args=(cmd,outfile))
    success=False
    theThread.start()
    sys.stderr.write("Waiting for thread to finish...\n")
    theThread.join(timeout)

    
    if theThread.isAlive():  
        sys.stderr.write( "ERROR: Command failed to exit within %d seconds!\n"%timeout)
        os.kill(proc.pid,9)
        errmsg = "Timeout"
        
    elif proc.returncode and proc.returncode < 0:
        errmsg = "Command returned exit code %d."%proc.returncode

    elif not os.path.exists(outfile):
        errmsg = "File %s was not created as expected.\n"%outfile 

    elif outfile[-3:] == ".sm":
        if  Popen("sminfo %s"%outfile, shell=True).wait():
            errmsg = "sminfo does not like the file %s"%outfile
        else:
            success = True            
    else:
        success=True
     
    
    sys.stderr.write("\n************************************************\n" )
    if success:
        sys.stderr.write("\nSuccess!\n" )
    else:
        sys.stderr.write("Failed.  Return code %s, reason: %s\n"%(str(retval),str(errmsg)))
    sys.stderr.write("\n************************************************\n\n" )
    return success


testrun("-v -ignore -form tiff  %s/mountains.tiff"%datadir, "%s/mountains-ignore.sm"%testdir)
testrun("-v -form png -first 084 -last 084 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png "%datadir, "%s/quicksand-single-template.sm"%testdir)
testrun("-v -form png -gz -first 20 -last 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png "%datadir, "%s/quicksand-all-template-gz.sm"%testdir)
testrun("-v -form png -lzma -first 20 -last 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png "%datadir, "%s/quicksand-all-template-lzma.sm"%testdir)

sys.exit(0)


