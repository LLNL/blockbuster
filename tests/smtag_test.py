#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

parser = argparse.ArgumentParser()
parser.add_argument('-b', '--bindir', help="set directory where smtag lives", default=test_common.FindBinDir('smtag'))

args = parser.parse_args()

[bindir,img2sm,datadir] = test_common.FindPaths(args.bindir, "smtag")

