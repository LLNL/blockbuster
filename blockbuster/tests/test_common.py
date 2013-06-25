#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse
from subprocess import *

gTestdir = "/tmp/"+os.getenv("USER")
gBindir = "" # almost certainly bad
gDatadir = "" # almost certainly bad

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
proc = None
def run_command(cmd, outfile):
    global proc
    sys.stderr.write( "Running command: \"%s\"\n"%cmd)
    #sys.stderr.write("length of command: %d\n"%len(cmd))
    proc = Popen(cmd.split(), bufsize=-1, stdout=outfile, stderr=STDOUT)
    proc.wait()
    return 

# =================================================================
# python 2.7 has check_output, but we are assuming 2.6 here
def CheckOutput(cmd):
    outfilename = "tmpfile.out"
    outfile = open(outfilename, "w")
    run_command(cmd, outfile)
    outfile.close()
    return open(outfilename, 'r').read()

# =================================================================
def FindBinDir(progname):
    global gBindir
    # try ../$SYS_TYPE/bin (usual case) and ../../$SYS_TYPE/bin (dev on LC)
    for dots in ['..','../..','../../..']:
        for subdir in [systype, '.']:
            trydir = "%s/%s/%s/bin"%(testdir,dots,subdir)
            # sys.stderr.write( "trying directory: %s\n"%trydir)
            if  os.path.exists(trydir+'/' + progname):
                gBindir = trydir
                return gBindir         
    gBindir = "%s/../%s/bin"%(testdir,systype)
    return gBindir

# ============================================================================================
def FindDataDir():
    global gDatadir
    gDatadir = os.path.abspath(os.path.dirname(sys.argv[0])+'/../sample-data')
    if not os.path.exists(gDatadir):
        proc = Popen(("tar -C %s -xzf %s.tgz"%(gDatadir.replace('sample-data',''), gDatadir)).split())
        proc.wait()
    if not os.path.exists(gDatadir):
        errexit("Cannot find or create data dir %s"%gDatadir)
    return gDatadir

# =================================================================
def FindPaths(bindir, binaries):
    global gBindir, gDatadir
    if type(binaries) != type((1,2)) and type(binaries) != type([1,2]):
        binaries = [binaries]
    # print "FindBinary(%s, %s)"%(bindir,binary)
    if bindir:
        if not os.path.exists(bindir):
            errexit("bindir %s does not exist."%bindir)
        # fix relative paths and stuff:
        if bindir[-1] != '/':
            bindir = bindir + "/" 
        if bindir[0] != '/':
            bindir = os.getcwd() + '/' + bindir
        binary = "%s/%s"%(bindir,binaries[0])   
        bindir = os.path.realpath(os.path.abspath(os.path.dirname(binary)))
        gBindir = bindir
    try:
        i=len(binaries)
        while i:
            i = i-1
            binary = gBindir + '/' + binaries[i]
            binary = CheckOutput("which %s"%binary).strip()
            if not os.path.exists(binary):
                errexit( "Error: os.path() could not find binary %s"%binary)
            binaries[i] = binary
    except:
        sys.stderr.write( "Error: 'which' could not find binary %s\n"%binary)        
        raise
    
    gDatadir = FindDataDir()
    
    sys.stderr.write( "bindir is: %s\n"%bindir)
    sys.stderr.write( "binaries are: %s\n"% binaries)
    sys.stderr.write( "datadir is: %s\n"%gDatadir)
    
    return [bindir,binaries,gDatadir]

# ===================================================================
def CreateDir(outdir, clean=False):
    # CREATE OUTPUT DIRECTORY
    if os.path.exists(outdir) and clean:
        shutil.rmtree(outdir, ignore_errors=True) 
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    if not os.path.exists(outdir):
        errexit("Cannot create test output directory "+outdir)
    return

# ===================================================================
def SetBaseDir(basedir, clean=True):
    global gTestdir
    gTestdir = basedir
    CreateDir(gTestdir, clean=clean)
    return

# ================================================================
def run_test(test, timeout=15):
    global proc, gTestdir, gDatadir
    errmsg = "SUCCESS"
    os.chdir(gTestdir)
    print "run_test, cwd is %s, running test: %s"%(os.getcwd(), str(test))
    need_data = test['need_data']
    if need_data:
        print "need data:", need_data
        if not os.path.exists(need_data):
            src_data = gDatadir+'/'+need_data
            if not os.path.exists(src_data):
                errmsg = "Error: Cannot find or copy needed data %s"% need_data
            else:
                dest = gTestdir+"/"+need_data
                print "copying", src_data, "to", dest
                if os.path.isdir(src_data):
                    shutil.copytree(src_data, dest)
                else:
                    shutil.copy(src_data, dest)
                print "copied data to", dest

    outfilename = gTestdir+"%s.out"%test['name']
    outfile = open(outfilename, "w")
    outfile.write(test['cmd']+'\n')
    outfile.flush()
    if errmsg == "SUCCESS":
        theThread = threading.Thread(target=run_command, args=([test['cmd'], outfile]))
        theThread.start()
        sys.stderr.write("Waiting %d seconds for thread to finish...\n"%timeout)
        theThread.join(timeout)
        if theThread.isAlive():
            os.kill(proc.pid,9)
            errmsg = "ERROR: Command failed to exit within timeout %d seconds!\n"%timeout
        print "command output saved in", outfilename
    
        
    if errmsg == "SUCCESS" and proc and proc.returncode and proc.returncode < 0:
        errmsg = "Command returned exit code %d."%proc.returncode
        
    if errmsg == "SUCCESS" and test['output'] and not os.path.exists(test['output']):
        errmsg = "Output file %s was not created as expected.\n"%test['output']

    outfile.write(errmsg+'\n')
    outfile.close()
    if errmsg == "SUCCESS" and test['success_pattern'] or test['failure_pattern']:
        outfile = open(outfilename, 'r')
        found_failure = False
        found_success = False
        while not found_success and not found_failure:
            line = outfile.readline()
            if not line:
                break
            if test['success_pattern'] in line:
                found_success=True
            if test['failure_pattern'] in line:
                found_failure=True                
        if not found_success:
            errmsg = "Expected success pattern \"%s\" not found in output."%test['success_pattern']
        if found_failure:
            errmsg = "Found failure pattern \"%s\" in output."%test['failure_pattern']
            
    if errmsg == "SUCCESS":
        resultstring = errmsg
    else:
        returncode = None
        if proc:
            returncode = proc.returncode
        result = [False, errmsg]
        resultstring = "Failed.  Return code %s, reason: \"%s\"\n"%(str(returncode),errmsg)
        
    sys.stderr.write("\n"+"*"*50+"\n" )
    sys.stderr.write("%s\n"%resultstring)       
    sys.stderr.write("\n"+"*"*50+"\n\n" )
    
    return  [errmsg == "SUCCESS", errmsg]
    return [True, ["No errors"]]

# ============================================================================================
def RunTests(tests):
    successes = 0
    results = []
    for test in tests:
        result = run_test(test)
        results.append(result)
        successes = successes + result[0]

    outfilename = os.getcwd() + "/" + "results.out"
    outfile = open(outfilename, "w")
    resultstring = "*"*50+"\n" + "successes:  %d out of %d tests\n"%(successes, len(tests)) + "results: " + str(results) + "\n"+"*"*50+"\n"
    print resultstring
    outfile.write(resultstring + "\n")
    print "Results saved in", outfilename
    outfile.close()
    return [successes, results]

