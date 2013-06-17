#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = argparse.ArgumentParser()
parser.add_argument('-b', '--bindir', help="set directory where img2sm lives", default=test_common.FindBinDir('img2sm'))

args = parser.parse_args()

fixed = test_common.CheckAndFixDir(args.bindir)
if not fixed:
    errexit("bindir %s does not exist.  Please use the --bindir argument."%bindir)
bindir = fixed

img2sm = test_common.FindBinary(bindir, "img2sm")

bindir = os.path.abspath(os.path.dirname(img2sm))
sys.stderr.write( "bindir is: %s\n"%bindir)
sys.stderr.write( "Found img2sm at %s"% img2sm)

# FIND DATA DIR
datadir = os.path.abspath(os.path.dirname(sys.argv[0])+'/../sample-data')
sys.stderr.write( "datadir is "+ datadir)
if not os.path.exists(datadir):
    proc = Popen(("tar -C %s -xzf %s.tgz"%(datadir.replace('sample-data',''), datadir)).split())
    proc.wait()
if not os.path.exists(datadir):
    errexit("Cannot find or create data dir %s"%datadir)
    
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


