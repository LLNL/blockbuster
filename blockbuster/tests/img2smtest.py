#!/usr/bin/env python

import sys, os, test_common
parser = test_common.get_arg_parser()
args = parser.parse_args()

test_common.FindPaths(args.bindir)

# ============================================================================================
# DEFINE TESTS
IMG2SM_SUCCESS = "img2sm successfully created movie"
IMG2SM_FAILURE = "ERROR"
SMQUERY_SUCCESS = None
SMQUERY_FAILURE = None

tests = [ {"name": "mountains-single",
           "need_data": "mountains.tiff", 
           "cmd": "img2sm",
           "args": "-v 5 mountains.tiff mountains.sm",
           "output": "mountains.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS},
          {"name": "check-mountains-single", 
           "need_data": "mountains.sm", 
           'cmd': "sminfo",
           "args": "mountains.sm",
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},
          
          {"name": "quicksand-single-gz",
           "need_data": "quicksand-short-6fps", 
           "cmd": "img2sm",
           "args": "-v 5 --first 084 --last 084 quicksand-short-6fps/quicksand-short-6fps%03d.png quicksand-single-gz.sm", 
           "output": "quicksand-single-gz.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern":  IMG2SM_SUCCESS}, 
          {"name": "check-quicksand-single-gz", 
           "need_data": "quicksand-single-gz.sm", 
           'cmd': "sminfo",
           "args": "quicksand-single-gz.sm",
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},
          
          {"name": "quicksand-11frames-gz",
           "need_data": "quicksand-short-6fps", 
           "cmd": "img2sm",
           "args": "-v 5 -c gz --first 20 -l 30 quicksand-short-6fps/quicksand-short-6fps%03d.png quicksand-11frames-gz.sm", 
           "output": "quicksand-11frames-gz.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS},           
          {"name": "check-quicksand-11frames-gz",
           "need_data": "quicksand-11frames-gz.sm", 
           'cmd': "sminfo",
           "args": "quicksand-11frames-gz.sm",
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},
          
          {"name": "quicksand-11frames-lzma",
           "need_data": "quicksand-short-6fps", 
           "cmd": "img2sm",
           "args": "-v 5 --compression lzma --first 20 --last 30 quicksand-short-6fps/quicksand-short-6fps%03d.png quicksand-11frames-lzma.sm",
           "output": "quicksand-11frames-lzma.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS },           
          {"name": "check-quicksand-11frames-lzma",
           "need_data": "quicksand-11frames-lzma.sm", 
           'cmd': "sminfo",
           "args": "quicksand-11frames-lzma.sm", 
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},
          
          {"name": "quicksand-wildcard-11frames-lzma",
           "need_data": "quicksand-short-6fps", 
           "cmd": "img2sm",
           "args": "-v 5 --compression lzma --first 20 --last 30 quicksand-short-6fps/quicksand-short-*.png quicksand-wildcard-11frames-lzma.sm",
           "output": "quicksand-wildcard-11frames-lzma.sm",
           "failure_pattern": IMG2SM_FAILURE,
           "success_pattern": IMG2SM_SUCCESS },           
          {"name": "check-quicksand-wildcard-11frames-lzma",
           "need_data": "quicksand-wildcard-11frames-lzma.sm", 
           'cmd': "sminfo",
           "args": "quicksand-wildcard-11frames-lzma.sm", 
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": SMQUERY_SUCCESS},
          
          {"name": "tagged-quicksand-11frames-lzma",
           "need_data": "quicksand-wildcard-11frames-lzma.sm", 
           'cmd': "sminfo",
           "args": "quicksand-wildcard-11frames-lzma.sm", 
           "output": None,
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": ["Movie Creator.*%s"%os.getenv("USER"), "Movie Create Host.*%s"%os.getenv("HOST"), "Movie Create Date"]}, 

           {"name": "smtag-quicksand-11frames-lzma",
           "need_data": "quicksand-wildcard-11frames-lzma.sm", 
           'cmd': "smtag",
           "args": "-v 5 -T 'testtag: 78 :INT64' -E smtag-quicksand.tagfile quicksand-wildcard-11frames-lzma.sm", 
           "output": "smtag-quicksand.tagfile",
           "failure_pattern": SMQUERY_FAILURE,
           "success_pattern": "SetValue\(METADATA_TYPE_INT64, 78\)" }

          
         ]

# ============================================================================================
# RUN TESTS
[successes, results] = test_common.RunTests(tests)

if successes != len(tests):
    sys.exit(1)

sys.exit(0)


