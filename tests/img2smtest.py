#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = argparse.ArgumentParser()
parser.add_argument('-b', '--bindir', help="set directory where img2sm lives", default=test_common.FindBinDir('img2sm'))

args = parser.parse_args()

basedir = "/tmp/"+os.getenv("USER")+"/img2smtest/"
test_common.SetBaseDir(basedir)

[bindir,[img2sm,sminfo],datadir] = test_common.FindPaths(args.bindir, ["img2sm","sminfo"])

# ============================================================================================
# DEFINE TESTS
IMG2SM_SUCCESS = "img2sm completed successfully."
IMG2SM_FAILURE = "ERROR"
SMQUERY_SUCCESS = None
SMQUERY_FAILURE = None

tests = [ {"name": "mountains-single",
           "need_data": "mountains.tiff", 
           "cmd": "%s -v mountains.tiff %s"%(img2sm, "mountains.sm"),
           "output": "mountains.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS},
          {"name": "check-mountains-single", 
           "need_data": "mountains.sm", 
           'cmd': "%s mountains.sm"%sminfo,
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},
          {"name": "quicksand-single-gz",
           "need_data": "quicksand-short-6fps", 
           "cmd": "%s -v --first 084 --last 084 quicksand-short-6fps/quicksand-short-6fps%%03d.png quicksand-single-gz.sm"%img2sm, 
           "output": "quicksand-single-gz.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern":  IMG2SM_SUCCESS}, 
          {"name": "check-quicksand-single-gz", 
           "need_data": "quicksand-single-gz.sm", 
           'cmd': "%s quicksand-single-gz.sm"%sminfo,
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},           
          {"name": "quicksand-11frames-gz",
           "need_data": "quicksand-short-6fps", 
           "cmd": "%s -v -c gz --first 20 -l 30 quicksand-short-6fps/quicksand-short-6fps%%03d.png quicksand-11frames-gz.sm"%img2sm, 
           "output": "quicksand-11frames-gz.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS},           
          {"name": "check-quicksand-11frames-gz",
           "need_data": "quicksand-11frames-gz.sm", 
           'cmd': "%s quicksand-11frames-gz.sm"%sminfo,
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},        
          {"name": "quicksand-11frames-lzma",
           "need_data": "quicksand-short-6fps", 
           "cmd": "%s -v --compression lzma --first 20 --last 30 quicksand-short-6fps/quicksand-short-6fps%%03d.png quicksand-11frames-lzma.sm"%img2sm,
           "output": "quicksand-11frames-lzma.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS },           
          {"name": "check-quicksand-11frames-lzma",
           "need_data": "quicksand-11frames-lzma.sm", 
           'cmd': "%s quicksand-11frames-lzma.sm"%sminfo, 
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS}       
         ]

# ============================================================================================
# RUN TESTS
[successes, results] = test_common.RunTests(tests)

print "output is in", basedir

if successes != len(tests):
    sys.exit(1)

sys.exit(0)


