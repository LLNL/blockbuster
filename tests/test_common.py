#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, test_common
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
            binary in binaries[i]
            binary = CheckOutput("which %s"%binary).strip()
            if not os.path.exists(binary):
                errexit( "Error: os.path() could not find binary %s"%binary)
            binaries[i] = binary
    except:
        sys.stderr.write( "Error: 'which' could not find binary %s\n"%binary)        
        raise
    
    gDatadir = FindDataDir()
    
    sys.stderr.write( "bindir is: %s\n"%bindir)
    sys.stderr.write( "binary is: %s\n"% binary)
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
    test_common.CreateDir(gTestdir, clean=clean)
    return

# ================================================================
def run_test(test, timeout=15):
    global proc, gTestdir, gDatadir
    errmsgs = []
    os.chdir(gTestdir)
    print "run_test, cwd is %s, running test: %s"%(os.getcwd(), str(test))
    need_data = test['need_data']
    if need_data:
        print "need data:", need_data
        if not os.path.exists(need_data):
            src_data = gDatadir+'/'+need_data
            if not os.path.exists(src_data):
                errmsgs.append("Error: Cannot find or copy needed data %s"% need_data)
            else:
                dest = gTestdir+"/"+need_data
                print "copying", src_data, "to", dest
                if os.path.isdir(src_data):
                    shutil.copytree(src_data, dest)
                else:
                    shutil.copy(src_data, dest)
                print "copied data to", dest

    outfilename = gTestdir+"%s.out"%test['name']
    if not errmsgs:
        outfile = open(outfilename, "w")
        theThread = threading.Thread(target=run_command, args=([test['cmd'], outfile]))
        theThread.start()
        sys.stderr.write("Waiting %d seconds for thread to finish...\n"%timeout)
        theThread.join(timeout)
        outfile.close()
        if theThread.isAlive():
            os.kill(proc.pid,9)
            errmsgs.append("ERROR: Command failed to exit within timeout %d seconds!\n"%timeout)            
        print "command output saved in", outfilename
    
        
    if proc and proc.returncode and proc.returncode < 0:
        errmsgs.append("Command returned exit code %d."%proc.returncode)
        
    if test['output'] and not os.path.exists(test['output']):
        errmsgs.append("Output file %s was not created as expected.\n"%test['output'])
                       
    if test['expect_output']:
        outfile = open(outfilename, 'r')
        found = False
        while not found:
            line = outfile.readline()
            if not line:
                break
            if test['expect_output'] in line:
                found=True
                break
        if not found:
            errmsgs.append("Expected output \"%s\" not found in output."%test['expect_output'])

    sys.stderr.write("\n************************************************\n" )
    if not errmsgs:
        sys.stderr.write("\nSuccess! \n" )
    else:
        returncode = None
        if proc:
            returncode = proc.returncode
        sys.stderr.write("Failed.  Return code %s, reasons: %s\n"%(str(returncode),str(errmsgs)))
    sys.stderr.write("\n************************************************\n\n" )
    if errmsgs:
        return  [False, errmsgs]
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

