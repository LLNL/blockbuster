#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = argparse.ArgumentParser()
parser.add_argument('-b', '--bindir', help="set directory where img2sm lives", default=test_common.FindBinDir('img2sm'))

args = parser.parse_args()

outdir = "/tmp/"+os.getenv("USER")+"/img2smtest/"
test_common.CreateEmptyDir(outdir)

[bindir,img2sm,datadir] = test_common.FindPaths(args.bindir, "img2sm")

# ============================================================================================
# DEFINE TESTS
tests = [ {"name": "mountains-single",
           "cmd": "%s -v %s/mountains.tiff %s"%(img2sm, datadir, "%s/mountains-ignore.sm"%outdir),
           "output": "%s/mountains-ignore.sm"%outdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/mountains-ignore.sm"%outdir)},
          {"name": "quicksand-single-gz",
           "cmd": "%s -v --first 084 --last 084 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png %s"%(img2sm, datadir, "%s/quicksand-single-template.sm"%outdir), 
           "output": "%s/quicksand-single-template.sm"%outdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/quicksand-single-template.sm"%outdir)},
          {"name": "quicksand-11frames-gz",
           "cmd": "%s -v -c gz --first 20 -l 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png %s"%(img2sm, datadir, "%s/quicksand-all-template-gz.sm"%outdir), 
           "output": "%s/quicksand-all-template-gz.sm"%outdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/quicksand-all-template-gz.sm"%outdir)},
          {"name": "quicksand-11frames-lzma",
           "cmd": "%s -v --compression lzma --first 20 --last 30 %s/quicksand-short-6fps/quicksand-short-6fps%%03d.png %s"%(img2sm, datadir, "%s/quicksand-all-template-lzma.sm"%outdir), 
           "output": "%s/quicksand-all-template-lzma.sm"%outdir,
           'check_cmd': "%s/sminfo %s"%(bindir,"%s/quicksand-all-template-lzma.sm"%outdir)}
          ]

# ============================================================================================
# RUN TESTS
[successes, results] = test_common.RunTests(tests)

print "output is in", outdir

if successes != len(tests):
    sys.exit(1)

sys.exit(0)


