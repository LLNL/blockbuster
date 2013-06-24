#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = argparse.ArgumentParser()
parser.add_argument('-b', '--bindir', help="set directory where smtag lives", default=test_common.FindBinDir('smtag'))

args = parser.parse_args()

outdir = "/tmp/"+os.getenv("USER")+"/smtagtests/"
test_common.CreateEmptyDir(outdir)

[bindir,smtag,datadir] = test_common.FindPaths(args.bindir, "smtag")

# ==========================================================================
# DEFINE TESTS
tests = [ {"name": "basic-tagging",
           "cmd": "%s -v %s/mountains.tiff %s"%(smtag, datadir, "%s/mountains-ignore.sm"%outdir),
           "output": "%s/mountains-ignore.sm"%outdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/mountains-ignore.sm"%outdir)}
          ]

# ==========================================================================
# RUN TESTS
[successes, results] = test_common.RunTests(tests)

print "output is in", outdir

if successes != len(tests):
    sys.exit(1)

sys.exit(0)

