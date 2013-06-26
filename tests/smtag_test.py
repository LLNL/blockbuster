#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = test_common.get_arg_parser()
args = parser.parse_args()

[bindir,smtag,datadir] = test_common.FindPaths(args.bindir, "smtag")

# ==========================================================================
# DEFINE TESTS
tests = [
    {"name": "mountains-single",
     "need_data": "mountains.tiff", 
     "cmd": "%s -v mountains.tiff %s"%(img2sm, "mountains.sm"),
     "output": "mountains.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS},
    {"name": "mountains-single",
     "need_data": "mountains.tiff", 
     "cmd": "%s -v mountains.tiff %s"%(img2sm, "mountains.sm"),
     "output": "mountains.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS}
    ]

# ==========================================================================
# RUN TESTS
[successes, results] = test_common.RunTests(tests)

print "output is in", outdir

if successes != len(tests):
    sys.exit(1)

sys.exit(0)

