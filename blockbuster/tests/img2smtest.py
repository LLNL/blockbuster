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
    for dots in ['..','../..','../../..']:
        trydir = "%s/%s/%s/bin"%(os.path.abspath(os.path.dirname(sys.argv[0])),dots,systype)
        sys.stderr.write( "trying directory: %s\n"%trydir)
        if  os.path.exists(trydir+'/sm2img'):
            bindir=trydir
            break
if bindir:
    sys.stderr.write( "found bindir : %s\n"%trydir)
else:
    sys.stderr.write("Could not find bindir\n")
    sys.exit(1)
    
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
sys.stderr.write( "bindir is: %s\n"%bindir)
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
    
def run_img2sm(cmd,outfile):
    global proc
    cmd="%s %s %s"%(img2sm, cmd,outfile) 
    sys.stderr.write( "Running test: \"%s\\n"%cmd)
    sys.stderr.write("lengh of command: %d\n"%len(cmd))
    proc = Popen(cmd.split(), bufsize=-1)
    proc.wait()
    return

def test_img2sm(test, timeout=5):
    global success, errmsg, proc
    args = test['args']
    outfile = test['output']
    theThread = threading.Thread(target=run_img2sm,args=(args,outfile))
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
        if  Popen("%s/sminfo %s"%(bindir,outfile), shell=True).wait():
            errmsg = "%s/sminfo does not like the file %s"%(bindir,outfile)
        else:
            success = True            
    else:
        success=True
     
    
    sys.stderr.write("\n************************************************\n" )
    if success:
        sys.stderr.write("\nSuccess!\n" )
    else:
        sys.stderr.write("Failed.  Return code %s, reason: %s\n"%(str(proc.returncode),str(errmsg)))
    sys.stderr.write("\n************************************************\n\n" )
    return success

tests = [ {"name": "mountains-single",
           "args": "-v   %s/mountains.tiff"%datadir,
           "output": "%s/mountains-ignore.sm"%testdir},
          {"name": "quicksand-single-gz",
           "args": "-v  --first 084 --last 084 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png "%datadir, 
           "output": "%s/quicksand-single-template.sm"%testdir},
          {"name": "quicksand-11frames-gz",
           "args": "-v  -c gz --first 20 -l 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png "%datadir, 
           "output": "%s/quicksand-all-template-gz.sm"%testdir},
          {"name": "quicksand-11frames-lzma",
           "args": "-v --compression lzma --first 20 --last 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png "%datadir, 
           "output": "%s/quicksand-all-template-lzma.sm"%testdir}
          ]
successes = 0
results = []
for test in tests:
    result = test_img2sm(test)
    results.append(result)
    successes = successes + result

print "****************************************************\n"
print "successes:  %d out of %d tests\n"%(successes, len(tests))
print "results:", results
print "\n****************************************************\n"

if successes != len(tests):
    sys.exit(1)


# Next:  sm2img

sys.exit(0)


