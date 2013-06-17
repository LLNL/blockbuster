#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = argparse.ArgumentParser()
parser.add_argument('-b', '--bindir', help="set directory where img2sm lives", default=test_common.FindBinDir('img2sm'))

args = parser.parse_args()

[bindir,img2sm,datadir] = test_common.FindPaths(args.bindir, "img2sm")

# CREATE OUTPUT DIRECTORY
testdir = "/tmp/"+os.getenv("USER")+"/img2smtest/"
shutil.rmtree(testdir, ignore_errors=True) 
os.makedirs(testdir)
if not os.path.exists(testdir):
    errexit("Cannot create test output directory "+testdir)

# ============================================================================================
# RUN TESTS
tests = [ {"name": "mountains-single",
           "cmd": "%s -v   %s/mountains.tiff %s"%(img2sm, datadir, "%s/mountains-ignore.sm"%testdir),
           "output": "%s/mountains-ignore.sm"%testdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/mountains-ignore.sm"%testdir)},
          {"name": "quicksand-single-gz",
           "cmd": "%s -v  --first 084 --last 084 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png %s"%(img2sm, datadir, "%s/quicksand-single-template.sm"%testdir), 
           "output": "%s/quicksand-single-template.sm"%testdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/quicksand-single-template.sm"%testdir)},
          {"name": "quicksand-11frames-gz",
           "cmd": "%s -v  -c gz --first 20 -l 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png %s"%(img2sm, datadir, "%s/quicksand-all-template-gz.sm"%testdir), 
           "output": "%s/quicksand-all-template-gz.sm"%testdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/quicksand-all-template-gz.sm"%testdir)},
          {"name": "quicksand-11frames-lzma",
           "cmd": "%s -v --compression lzma --first 20 --last 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png %s"%(img2sm, datadir, "%s/quicksand-all-template-lzma.sm"%testdir), 
           "output": "%s/quicksand-all-template-lzma.sm"%testdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/quicksand-all-template-lzma.sm"%testdir)}
          ]
successes = 0
results = []
for test in tests:
    result = test_common.run_test(test)
    results.append(result)
    successes = successes + result[0]
    
print "****************************************************\n"
print "successes:  %d out of %d tests\n"%(successes, len(tests))
print "results:", results
print "\n****************************************************\n"

if successes != len(tests):
    sys.exit(1)

sys.exit(0)


