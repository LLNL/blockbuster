#!/usr/bin/env python

import sys, os, test_common, argparse
parser = test_common.get_arg_parser()
parser.add_argument('-s', '--stop-on-failure', help="Stop testing when a failure occurs", action='store_true', default=None)
parser.add_argument('-g', '--create-gold-standard', help="Create baseline images and files for use in later testing.  Assumes the current code is perfect", action='store_true')
args = parser.parse_args()

if args.bindir != "":
    test_common.SetBindir(args.bindir)
if args.output_dir != "":
    test_common.SetOutputdir(args.output_dir)

# ==============================================================
# DEFINE TESTS
IMG2SM_SUCCESS = "img2sm successfully created movie"
IMG2SM_FAILURE = ["ERROR", "UNKNOWN"]
SMQUERY_SUCCESS = None
SMQUERY_FAILURE = ["ERROR", "UNKNOWN"]
SCRIPT_SUCCESS = None
SCRIPT_FAILURE = ["ERROR"]
BLOCKBUSTER_SUCCESS = None
BLOCKBUSTER_FAILURE = ["ERROR"]
SMCAT_FAILURE = ["ERROR"]
SMCAT_SUCCESS = "smcat successfully created movie "

tests = [
    # ===============================================       
    {"name": "mountains-single",
     "need_data": "mountains.tiff", 
     "cmd": "img2sm",
     "args": "-v 5 mountains.tiff mountains.sm",
     "output": "mountains.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS,
     "frame diffs": ["mountains.sm",0]
     },
    # --------------------------------------------
    {"name": "check-mountains-single", 
     "need_data": "mountains.sm", 
     'cmd': "sminfo",
     "args": "mountains.sm",
     "output": None,
    "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": SMQUERY_SUCCESS,
     },
    
    # ===============================================       
    {"name": "stereo-lowercase-template",
     "need_data": "stereo-frames", 
     "cmd": "img2sm",
     "args": "-v 5 --Stereo 'stereo-frames/<LR>_stereo-test%04d.png' stereo-lowercase-template.sm",
     "output": "stereo-lowercase-template.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS,
     "frame diffs": ["stereo-lowercase-template.sm",0]
     },
    # ===============================================       
    {"name": "stereo-uppercase-glob",
     "need_data": "stereo-frames", 
     "cmd": "img2sm",
     "args": "-v 5 --Stereo stereo-frames/*T__st*png stereo-uppercase-glob.sm",
     "output": "stereo-uppercase-glob.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS,
     "frame diffs": ["stereo-uppercase-glob.sm",0]
     },
    # ===============================================       
    {"name": "quicksand-single-gz",
     "need_data": "quicksand-short-6fps", 
     "cmd": "img2sm",
     "args": "-v 5 --first 084 --last 084 quicksand-short-6fps/quicksand-short-6fps%03d.png quicksand-single-gz.sm", 
     "output": "quicksand-single-gz.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern":  IMG2SM_SUCCESS  ,
     "frame diffs": ["quicksand-single-gz.sm", 0]
     },
    # --------------------------------------------          
    {"name": "check-quicksand-single-gz", 
     "need_data": "quicksand-single-gz.sm", 
     'cmd': "sminfo",
     "args": "quicksand-single-gz.sm",
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": SMQUERY_SUCCESS
     },
    
    # ===============================================       
    {"name": "quicksand-11frames-gz",
     "need_data": "quicksand-short-6fps", 
     "cmd": "img2sm",
    "args": "-v 5 -c gz -T sometag:somevalue --first 20 -l 30 quicksand-short-6fps/quicksand-short-6fps%03d.png quicksand-11frames-gz.sm", 
     "output": "quicksand-11frames-gz.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS,
     "frame diffs": ["quicksand-11frames-gz.sm", 5]
     },
    # --------------------------------------------          
    {"name": "check-quicksand-11frames-gz",
     "need_data": "quicksand-11frames-gz.sm", 
     'cmd': "sminfo",
     "args": "quicksand-11frames-gz.sm",
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": SMQUERY_SUCCESS
     },
    
    # --------------------------------------------          
    # Make sure that adding tags to a movie do not cause the movie to be unviewable
    { "name": "blockbuster-test-img2sm-safety",
      "need_data": ["quicksand-11frames-gz.sm",
                    "blockbuster-test-img2sm-safety.script"],
      "cmd": "blockbuster",
      "args": "--script blockbuster-test-img2sm-safety.script -v 5",
      "output": ["quicksand-11frames-gz-frame-10.png",
                 "quicksand-11frames-gz-frame-0.png"],
      "failure_pattern": BLOCKBUSTER_FAILURE,
      "success_pattern": ["bad magic .* at pos 654665",
                          "window 10: offset 594098 size 60591"],    
      "frame diffs": [["quicksand-11frames-gz-frame-10.png", -1],
                      ["quicksand-11frames-gz-frame-0.png", -1]],
      "return": 0
      },
    # ===============================================       
    {"name": "quicksand-11frames-lzma",
     "need_data": "quicksand-short-6fps", 
     "cmd": "img2sm",
     "args": "-v 5 --compression lzma --first 20 --last 30 quicksand-short-6fps/quicksand-short-6fps%03d.png quicksand-11frames-lzma.sm",
     "output": "quicksand-11frames-lzma.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS,
     "frame diffs": ["quicksand-11frames-lzma.sm",5]
     },           
    # --------------------------------------------
    {"name": "check-quicksand-11frames-lzma",
     "need_data": "quicksand-11frames-lzma.sm", 
     'cmd': "sminfo",
     "args": "quicksand-11frames-lzma.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": SMQUERY_SUCCESS
     },
    
    # ===============================================       
    {"name": "quicksand-wildcard-11frames-lzma",
     "need_data": "quicksand-short-6fps", 
     "cmd": "img2sm",
     "args": "-v 5 --compression lzma --first 20 --last 30 quicksand-short-6fps/quicksand-short-*.png quicksand-wildcard-11frames-lzma.sm",
     "output": "quicksand-wildcard-11frames-lzma.sm",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern": IMG2SM_SUCCESS,
     "frame diffs": ["quicksand-wildcard-11frames-lzma.sm",5]
     },           
    # --------------------------------------------
    {"name": "check-quicksand-wildcard-11frames-lzma",
     "need_data": "quicksand-wildcard-11frames-lzma.sm", 
     'cmd': "sminfo",
     "args": "quicksand-wildcard-11frames-lzma.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": SMQUERY_SUCCESS},
    
    # ===============================================       
    {"name": "steamboat-globbed-allframes-with-tagfile",
     "need_data": "steamboat", 
     'cmd': "img2sm",
     "args": "--report -T 'testtag2:steamboat' -E steamboat/*png steamboat-globbed-allframes.sm", 
     "output": "steamboat-globbed-allframes.tagfile",
     "failure_pattern": IMG2SM_FAILURE,
     "success_pattern":
     ["\( *ASCII\) testtag2 *= \"steamboat\"",
      "Movie Create Host.*%s"%os.getenv("HOST"),
      "Movie Create Date",
      "\( *ASCII\) Movie Creator *=.*%s"%os.getenv("USER")],
     "frame diffs": ["steamboat-globbed-allframes.sm",1]
     },
    
    # ===============================================       
    {"name": "tagged-quicksand-11frames-lzma",
     "need_data": "quicksand-wildcard-11frames-lzma.sm", 
     'cmd': "smquery",
     "args": "quicksand-wildcard-11frames-lzma.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern":
     ["Movie Creator.*%s"%os.getenv("USER"),
      "Movie Create Host.*%s"%os.getenv("HOST"),
      "Movie Create Date"]
     }, 
    
    # ===============================================       
    {"name": "smtag-filegen",
     "need_data": None, 
     'cmd': "smtag",
     "args": "-v 5 --report -T 'testtag: 78 :INT64' -T 'doubletag: 42.4 :DOUBLE' -T 'horsie tag:horse feathers are fluffy' -E", 
     "output": ["tags.tagfile"], 
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern":
     ["\( *DOUBLE\) doubletag *: value = 42.400000", 
      "\( *ASCII\) horsie tag *: value = \"horse feathers are fluffy\"", 
     "\( *INT64\) testtag *: value = 78"]
     },
    
    # ===============================================       
    {"name": "smtag-from-file",
     "need_data": ["quicksand-wildcard-11frames-lzma.sm",
                   "tags.tagfile"], 
     'cmd': "smtag",
     "args": "-v 5 --report -T 'testtag2: 82 :INT64' -T 'doubletag2: 47.4 :DOUBLE' -T 'horsie tag:new horsie tag' -E -F tags.tagfile quicksand-wildcard-11frames-lzma.sm", 
     "output": "quicksand-wildcard-11frames-lzma.tagfile",
     "failure_pattern": None,
     "success_pattern":
     ["\( *DOUBLE\) doubletag *: value = 42.400000", 
      "\( *DOUBLE\) doubletag2 *: value = 47.400000", 
       "\( *ASCII\) Title *: value = \"quicksand-wildcard-11frames-lzma.sm\"", 
      "\( *ASCII\) horsie tag *: value = \"new horsie tag\"", 
      "\( *INT64\) testtag *: value = 78", 
      "\( *INT64\) testtag2 *: value = 82"],
     "frame diffs": ["quicksand-wildcard-11frames-lzma.sm", 1]
    },
    
    # ===============================================
    # Make sure that -playexit works
    { "name": "blockbuster-playexit",
      "need_data": ["quicksand-wildcard-11frames-lzma.sm"],
      "cmd": "blockbuster",
      "args": "--playexit 9 -v 5 quicksand-wildcard-11frames-lzma.sm",
      "output": None,
      "failure_pattern": BLOCKBUSTER_FAILURE,
      "success_pattern": "window 10: offset 473312 size 43940",    
      "return": 0
      },
         
    # ===============================================
    # Make sure that adding tags to a movie do not cause the movie to be unviewable
    { "name": "blockbuster-test-smtag-safety",
      "need_data": ["quicksand-wildcard-11frames-lzma.sm",
                    "blockbuster-test-smtag-safety.script"],
      "cmd": "blockbuster",
      "args": "--script blockbuster-test-smtag-safety.script -v 5",
      "output": ["quicksand-wildcard-11frames-lzma-frame-10.png",
                 "quicksand-wildcard-11frames-lzma-frame-0.png"],
      "failure_pattern": BLOCKBUSTER_FAILURE,
      "success_pattern": "window 10: offset 473312 size 43940",    
      "frame diffs": [["quicksand-wildcard-11frames-lzma-frame-10.png", -1],
                      ["quicksand-wildcard-11frames-lzma-frame-0.png", -1]],
      "return": 0
      },
    # ===============================================       
     {"name": "query-and",
     "need_data": ["quicksand-wildcard-11frames-lzma.sm"], 
     'cmd': "smquery",
     "args": " -v 1 -T 'horsie' -T doubletag -A quicksand-wildcard-11frames-lzma.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": "Matched all tags" ,    
      "return": 0
      },
    # ===============================================       
     {"name": "query-and-failure",
     "need_data": ["quicksand-wildcard-11frames-lzma.sm"], 
     'cmd': "smquery",
     "args": " -v 1 -T 'horsie' -T badtag -A quicksand-wildcard-11frames-lzma.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": "Did not match all tags",
      "return": 1
      },
    # ===============================================       
    {"name": "img2sm-canonical-pexpect-steamboat",
     "need_data": "steamboat", 
     'cmd': "img2sm",
     "args": "steamboat/* --report -C -T steamtag:boats -E  img2sm-canonical-tags.sm",
     "output": ["img2sm-canonical-tags.tagfile", "img2sm-canonical-tags.sm"],
     "failure_pattern": SCRIPT_FAILURE + IMG2SM_FAILURE,
     "success_pattern":
     ["\( *ASCII\) steamtag * = \"boats\"", "\( *ASCII\) Science Type * = \"bogus\""],
     "pexpect": [["Please enter a value for key Title.*:.*:", "", "3"],
                 ["Please enter a value for key Science.*:.*:", "", "bogus"],
                 ["Please enter a value for key UCRL.*:.*:", "", "8"],
                 ["Please enter a value for key Sim CPUs.*:.*:", "", "10"],
                 ["Please enter a value for key Sim Cluster.*:.*:", "", ""],
                 ["Please enter a value for key Keywords.*:.*:", "", "salmon, trout"],
                 ["Please enter a value for key .*Creator.*:.*:", "", "6"],
                 ["Please enter a value for key .*Sim Date.*:.*:", "", "Thu 1am "],
                 ["Please enter a value for key .*Sim Duration.*:.*:", "", "12"],
                ["Please enter a value for key .*Create Date.*:.*:", "", "June 18, 1965 3AM "],
                 ["Please enter a value for key .*Create Host.*:.*:", "Invalid date.*", "m"],
                 ["Please enter a value for key .*Create Host.*:.*:", "Invalid date.*", "12"],
                 ["Please enter a value for key .*Create Date.*:.*:", "", "June 18 "],
                 ["Please enter a value for key .*Create Host.*:.*:", "Invalid date.*", "115"],
                 ["You entered a bad tag number.*Please enter a value for key .*Create Host.*:.*:", "Invalid date.*", "m"],
                 ["Please enter a value for key .*Create Host.*:.*:", "", "m"],
                 ["Please enter a value for key .*Create Host.*:.*:", "", "s"],
                 ],
     "frame diffs": ["img2sm-canonical-tags.sm",1]
     },
    
    # ===============================================
     {"name": "smcat-quicksand-versions",
     "need_data": ["quicksand-11frames-gz.sm", "quicksand-11frames-lzma.sm", "quicksand-single-gz.sm", "quicksand-wildcard-11frames-lzma.sm"], 
     'cmd': "smcat",
     "args": "quicksand*.sm smcat-quicksand-versions.sm", 
     "output": None,
     "failure_pattern": SMCAT_FAILURE,
     "success_pattern": SMCAT_SUCCESS
      },
    # gz|GZ|jpeg|JPEG|jpg|JPG|lzma|LZMA|lzo|LZO|raw|RAW|rle|RLE
    # ===============================================
     {"name": "smcat-quicksand-jpeg",
     "need_data": ["quicksand-11frames-gz.sm"], 
     'cmd': "smcat",
     "args": "-c jpeg quicksand-11frames-gz.sm quicksand-11frames-jpeg.sm", 
     "output": "quicksand-11frames-jpeg.sm",
     "failure_pattern": SMCAT_FAILURE,
     "success_pattern": SMCAT_SUCCESS
      },
    
    # ===============================================
     {"name": "smcat-quicksand-lzma",
     "need_data": ["quicksand-11frames-gz.sm"], 
     'cmd': "smcat",
     "args": "-c lzma quicksand-11frames-gz.sm quicksand-11frames-lzma.sm", 
     "output": "quicksand-11frames-lzma.sm",
     "failure_pattern": SMCAT_FAILURE,
     "success_pattern": SMCAT_SUCCESS
      },
    
    # ===============================================
     {"name": "smcat-quicksand-lzo",
     "need_data": ["quicksand-11frames-gz.sm"], 
     'cmd': "smcat",
     "args": "-c lzo quicksand-11frames-gz.sm quicksand-11frames-lzo.sm", 
     "output": "quicksand-11frames-lzo.sm",
     "failure_pattern": SMCAT_FAILURE,
     "success_pattern": SMCAT_SUCCESS
      },
    
    # ===============================================
     {"name": "smcat-quicksand-raw",
     "need_data": ["quicksand-11frames-gz.sm"], 
     'cmd': "smcat",
     "args": "-c RAW quicksand-11frames-gz.sm quicksand-11frames-raw.sm", 
     "output": "quicksand-11frames-raw.sm",
     "failure_pattern": SMCAT_FAILURE,
     "success_pattern": SMCAT_SUCCESS
      },
    
    # ===============================================
     {"name": "smcat-quicksand-rle",
     "need_data": ["quicksand-11frames-gz.sm"], 
     'cmd': "smcat",
     "args": "-c rle quicksand-11frames-gz.sm quicksand-11frames-rle.sm", 
     "output": "quicksand-11frames-rle.sm",
     "failure_pattern": SMCAT_FAILURE,
     "success_pattern": SMCAT_SUCCESS
      },
    
    # ===============================================       
     {"name": "smquery-multiple-files",
     "need_data": "smquery-multiple-files.sh.rawout.goldstandard", 
     'cmd': "smquery",
     "args": "*.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": ["mountains.sm.*mountains.sm",
                         "quicksand-11frames-gz.sm.*Level:0 size:320x240 tile:320x240"],
     "tagfile diffs": ["smquery-multiple-files.sh.rawout"]
      },
    # ===============================================       
     {"name": "smquery-multiple-files-nofilenames",
     "need_data": "smquery-multiple-files-nofilenames.sh.rawout.goldstandard", 
     'cmd': "smquery",
     "args": "-P *.sm", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": ["^\( ASCII\).*mountains.sm",
                         "^\( ASCII\) \"Res 0\".*Level:0 size:320x240 tile:320x240"], 
     "tagfile diffs": ["smquery-multiple-files-nofilenames.sh.rawout"]
      },
    # ===============================================       
     {"name": "smquery-canonical-list",
     "need_data": None, 
     'cmd': "smquery",
     "args": " -C", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": "\( *ASCII\) UCRL *: value = "      
      },
    # ===============================================       
     {"name": "smtag-canonical-list",
     "need_data": None, 
     'cmd': "smtag",
     "args": " -C", 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": "\( *ASCII\) UCRL *: value = "
      },
    # ===============================================
    {"name":  "smtest-integrity-test",
     "need_data":  None,
     "cmd": "smtest",
     "args":  "*.sm",
     "timeout": 30, 
     "output": None,
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": None
     }, 
    # ===============================================       
    {"name": "lorenz-tagfile",
     "need_data": ["lorenz.json.goldstandard"], 
     'cmd': "smtag",
     "args": " --report -T 'LorenzTag:Lorenz value' -L lorenz.json -F tags.tagfile *.sm", 
     "output": "lorenz.json",
     "failure_pattern": SMQUERY_FAILURE,
     "success_pattern": None,
     "frame diffs": ["img2sm-canonical-tags.sm",1],
     "tagfile diffs": ["lorenz.json"]
     },
    # ===============================================       
    
   ]

# ======================================================================
# RUN TESTS
[successes, results] = test_common.RunTests(tests, args.stop_on_failure, args.create_gold_standard)

if successes != len(tests):
    sys.exit(1)

sys.exit(0)


