#!/usr/bin/env python

import sys, os, shutil, time
from subprocess import *

def errexit (msg):
    print msg
    sys.exit(1)

def usage():
    print "Usage:  img2smtest.py [options]"
    print "Options:"
    print " -bindir dirnam:  path to the directory containing sm2img for disambiguation -- typically this will be $INSTALL_DIR/bin"
    
# PARSE ARGUMENTS:
bindir=""
if "-bindir" in sys.argv:
    pos = sys.argv.index('-bindir')
    if len(sys.argv) < pos+2:
        usage()
        sys.exit(-1)        
    bindir=sys.argv[pos+1] + "/" # make sure there's a trailing '/'

# RUN TESTS:
testdir = "/tmp/"+os.getenv("USER")+"/img2smtest/"
shutil.rmtree(testdir, ignore_errors=True) 
os.mkdir(testdir)
if not os.path.exists(testdir):
    errexit("Cannot create test output directory "+testdir)

def testrun(command, filename):
    print "Running test:  %s %s"%(command,filename)
    p = Popen("%s %s"%(command,filename), shell=True, stderr=PIPE, stdout=PIPE)

    secs = 0.0
    while p.poll() and secs < 15:
        time.sleep(0.20)
        secs = secs + 0.2

    if p.poll():
        print "ERROR: Command %s %s failed to exit within 30 seconds!"%(command,filename)
        p.terminate()
        sys.exit(1)

    print "stdout: ", p.stdout.read()
    print "stderr: ", p.stderr.read()
    if not os.path.exists(filename):
        print "ERROR: File %s was not created as expected."%filename
        sys.exit(2)

    if Popen("sminfo %s"%filename, shell=True).wait():
        print "ERROR:  sminfo does not like the file %s"%filename
        sys.exit(3)
        
    print "Success!" 
    return

testrun(bindir +"img2sm -ignore -form tiff  sample-data/mountains.tiff", "%s/mountains-ignore.sm"%testdir)
testrun(bindir +"img2sm  -form png -first 084 -last 084 sample-data/quicksand-short-6fps/quicksand-short-6fps%03d.png", "%s/quicksand-single-template.sm"%testdir)
testrun(bindir +"img2sm -form png -gz sample-data/quicksand-short-6fps/quicksand-short-6fps\%03d.png", "%s/quicksand-all-template-gz.sm"%testdir)

sys.exit(0)

#img2sm -ignore -form  svel_clip_geom_rear_hi_res.jpg /nfs/tmp2/rcook/img2smtest/${version}-svel-ignore.sm || errexit "Failed -ignore with version=$version"
#img2sm -version $version -form png -first 084 -last 084 ~/dataAndImages/langerframes/frame%03d.png /nfs/tmp2/rcook/img2smtest/${version}-langer-single-with-template.sm || errexit "Failed single-with-template with version=$version"
#img2sm -version $version -form png ~/dataAndImages/langerframes/frame%03d.png /nfs/tmp2/rcook/img2smtest/${version}-langer-multiple-with-template.sm || errexit "Failed multiple-with-template with version=$version"


