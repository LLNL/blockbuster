#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
from subprocess import *

# =================================================================
systype = os.getenv("SYS_TYPE")
testdir = os.path.abspath(os.path.dirname(sys.argv[0]))

if not systype:
    systype = ''

# =================================================================
def errexit (msg):
    print '*********************************************************************'
    print "Error: " + msg
    print '*********************************************************************'
    sys.exit(1)

# =================================================================
def FindBinDir(progname):
    # try ../$SYS_TYPE/bin (usual case) and ../../$SYS_TYPE/bin (dev on LC)
    for dots in ['..','../..','../../..']:
        for subdir in [systype, '.']:
            trydir = "%s/%s/%s/bin"%(testdir,dots,subdir)
            # sys.stderr.write( "trying directory: %s\n"%trydir)
            if  os.path.exists(trydir+'/' + progname):
                return trydir
    return "%s/../%s/bin"%(testdir,systype)

# ============================================================================================
def FindDataDir():
    datadir = os.path.abspath(os.path.dirname(sys.argv[0])+'/../sample-data')
    if not os.path.exists(datadir):
        proc = Popen(("tar -C %s -xzf %s.tgz"%(datadir.replace('sample-data',''), datadir)).split())
        proc.wait()
    if not os.path.exists(datadir):
        errexit("Cannot find or create data dir %s"%datadir)
    return datadir

# =================================================================
def FindPaths(bindir, binary):
    # print "FindBinary(%s, %s)"%(bindir,binary)
    if bindir:
        if not os.path.exists(bindir):
            errexit("bindir %s does not exist."%bindir)
        # fix relative paths and stuff:
        if bindir[-1] != '/':
            bindir = bindir + "/" 
        if bindir[0] != '/':
            bindir = os.getcwd() + '/' + bindir
            
        binary = "%s/%s"%(bindir,binary)   
        bindir = os.path.realpath(os.path.abspath(os.path.dirname(binary)))

    try:
        p = Popen("which %s"%binary, shell=True, stdout=PIPE, stderr=STDOUT)
        p.wait()
        binary = p.stdout.read().strip()
    except:
        sys.stderr.write( "Error: 'which' could not find binary %s\n"%binary)        
        raise
    
    if not os.path.exists(binary):
        errexit( "Error: os.path() could not find binary %s"%binary)

    datadir = FindDataDir()
    
    sys.stderr.write( "bindir is: %s\n"%bindir)
    sys.stderr.write( "binary is: %s\n"% binary)
    sys.stderr.write( "datadir is: %s\n"%datadir)
    
    return [bindir,binary,datadir]

# ============================================================================================
proc = None
def run_command(cmd):
    global proc
    sys.stderr.write( "Running test: \"%s\"\n"%cmd)
    #sys.stderr.write("length of command: %d\n"%len(cmd))
    proc = Popen(cmd.split(), bufsize=-1)
    proc.wait()
    return 

# ============================================================================================
def run_test(test, timeout=15):
    global proc
    errmsg = "No errors"
    outfile = test['output']
    theThread = threading.Thread(target=run_command, args=([test['cmd']]))
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

    elif outfile and not os.path.exists(outfile):
        errmsg = "File %s was not created as expected.\n"%outfile 

    elif test['check_cmd']:
        if  Popen(test['check_cmd'], shell=True).wait():
            errmsg = "FAILED: %s"%test['check_cmd']
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
    return  [success, errmsg, proc]

