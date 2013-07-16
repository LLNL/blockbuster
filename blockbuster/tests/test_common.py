#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, re, stat, pexpect, glob
import signal
from subprocess import *

# =================================================================
gTestdir = "/tmp/"+os.getenv("USER")+"/blockbuster_tests/"
gBindir = "" # almost certainly bad
gDatadir = "" # almost certainly bad
gDBFile = None
gDBFilename = None

# =================================================================
def SetDBFile(outfilename=None):
    global gDBFilename, gDBFile
    if not gDBFile:
        if not gDBFilename:
            if not outfilename:
                outfilename = os.getcwd() + "/" + "testing-results.out"
            gDBFilename = outfilename
            dbprint("Debug output file is %s\n"%gDBFilename)
        gDBFile = open(gDBFilename, "w")
    return

# =================================================================
def dbprint(msg, outfile=None):
    SetDBFile()
    if outfile:
        outfile.write(msg)
    sys.stdout.write(msg)
    gDBFile.write(msg)
    
# =================================================================
def FindBinDir(progname):
    global gBindir
    # try ../$SYS_TYPE/bin (usual case) and ../../$SYS_TYPE/bin (dev on LC)
    for dots in ['..','../..','../../..']:
        for subdir in [systype, '.']:
            trydir = "%s/%s/%s/bin"%(testdir,dots,subdir)
            # dbprint( "trying directory: %s\n"%trydir)
            if  os.path.exists(trydir+'/' + progname):
                gBindir = trydir
                return gBindir         
    gBindir = "%s/../%s/bin"%(testdir,systype)
    return gBindir

# =================================================================
def get_arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bindir', help="set directory where smtools live", default=FindBinDir('smtag'))
    return parser

# =================================================================
systype = os.getenv("SYS_TYPE")
testdir = os.path.abspath(os.path.dirname(sys.argv[0]))

if not systype:
    systype = ''

# =================================================================
def errexit (msg):
    dbprint('*********************************************************************\n')
    dbprint("Error: %s\n" % msg)
    dbprint('*********************************************************************\n')
    sys.exit(1)

# ================================================================
# Create a run script from the command, essentially to allow filename globbing in arguments
def CreateScript(cmd, filename, verbose=True):
    if os.path.exists(filename):
        os.remove(filename)
    script = open(filename, "w")
    script.write("#!/usr/bin/env bash \n");
    if verbose:
        script.write("set -xv\n");
        script.write("echo script running...\n");
    script.write(cmd + ' 2>&1 \n');
    script.close()
    os.chmod(filename, stat.S_IRUSR | stat.S_IXUSR)
    return

# ================================================================
def CheckOutput(outfile, success_patterns, failure_patterns):
    errmsg = "SUCCESS"
    outfile.flush()
    outfile.seek(0)
    found_failure = False
    found_successes = []
    found_all_successes = False
    success_patterns = MakeList(success_patterns)
    failure_patterns = MakeList(failure_patterns)
    compiled_success_patterns = MakeCompiledList(success_patterns)
    compiled_failure_patterns = MakeCompiledList(failure_patterns)
    for line in outfile:
        if found_all_successes and not found_failure:
            break
        for pattern in range(len(compiled_success_patterns)):
            if re.search(compiled_success_patterns[pattern], line) and pattern not in found_successes:
                found_successes.append(pattern)
            if len(found_successes) == len(success_patterns):
                found_all_successes = True
        for pattern in range(len(compiled_failure_patterns)):
            if re.search(compiled_failure_patterns[pattern], line):
                found_failure= failure_patterns[pattern]              
    if len(found_successes) == len(success_patterns):
        found_all_successes = True
    if found_failure:
        errmsg = "Found failure pattern \"%s\" in output."%found_failure
    elif not found_all_successes:
        errmsg = "Expected patterns not found in command output: \n"
    for pattern in range(len(success_patterns)):
        if pattern not in found_successes:
            errmsg = errmsg+"   %d: \"%s\"\n"%(pattern, str(success_patterns[pattern]))
    return errmsg

# =================================================================
# outfile is a file object
proc = None
def run_command(cmd, outfile):
    global proc
    dbprint( "Running command: \"%s\"\n"%cmd)
    proc = Popen(cmd, bufsize=-1, stdout=outfile, stderr=STDOUT)
    proc.wait()
    dbprint("run_command completed.\n")
    return 

# =================================================================
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
def FindPaths(bindir):
    global gBindir, gDatadir
    if bindir:
        if not os.path.exists(bindir):
            errexit("bindir %s does not exist."%bindir)
        # fix relative paths and stuff:
        if bindir[-1] != '/':
            bindir = bindir + "/" 
        if bindir[0] != '/':
            bindir = os.getcwd() + '/' + bindir
        binary = "%s/img2sm"%bindir   
        bindir = os.path.realpath(os.path.abspath(os.path.dirname(binary)))
        gBindir = bindir
    
    gDatadir = FindDataDir()
    
    dbprint( "bindir is: %s\n"%bindir)
    dbprint( "datadir is: %s\n"%gDatadir)
    
    return

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


# ===================================================================
def MakeList(thing):
    if not thing:
        return []
    if type(thing) != type([1,2]) and type(thing) != type((1,2)):
        return [thing]
    return list(thing)

# ===================================================================
def MakeCompiledList(thing):
    patterns = MakeList(thing)
    compiled = []
    for thing in patterns:
        compiled.append(re.compile(thing))
    return compiled

# ================================================================
def RunTestCommand(fullcmd, test, outfile):
    timeout = 15
    name = test['name']
    outfile.write(fullcmd+'\n')
    outfile.write("Working directory: %s\n"%os.getcwd())
    outfile.flush()
    
    scriptname = "%s/%s.sh"%(os.getcwd(), name)
    CreateScript(fullcmd, scriptname)
    dbprint("Full command is \"%s\", placed in script %s\n"%(fullcmd, scriptname))
    
    theThread = threading.Thread(target=run_command, args=([scriptname, outfile]))
    theThread.start()
    dbprint("Waiting %d seconds for thread to finish...\n"%timeout)
    theThread.join(timeout)
    if theThread.isAlive():
        os.kill(proc.pid,9)
        return "ERROR: Command failed to exit within timeout %d seconds!\n"%timeout
    return "SUCCESS"


# ================================================================
def FrameDiffs(test):
    errmsg = "SUCCESS"
    if "frame diffs" not in test.keys():
        return "SUCCESS"
    # extract the frame
    movie = test['frame diffs'][0]
    frame = test['frame diffs'][1]
    outframe = "%s_testframe.png"%test['name']
    fullcmd = "%s/sm2img --first %d --last %d %s %s"%(gBindir, frame, frame, movie, outframe)
    outfilename = "%s/%s"%(os.getcwd(), test['name']+'.frame_diff.txt')
    dbprint("FrameDiffs: outfile is %s\n"%outfilename)
    outfile = open(outfilename, "w")
    outfile.close()
    outfile = open(outfilename, "r+")
    run_command(fullcmd.split(), outfile)
    errmsg = CheckOutput(outfile, "Successful completion", "ERROR")
    if errmsg != "SUCCESS":
        return errmsg

    
    return errmsg

# ================================================================
# for each line in the script, line[0] is the expect, line[1] are errors, and line[2] is the response
def RunTestExpect(fullcmd, test, outfile):
    errmsg =  "SUCCESS"
    wrappername = "%s/%s.sh"%(os.getcwd(), test['name'])
    CreateScript(fullcmd, wrappername)
    script = test['pexpect']
    dbprint("Running command in pexpect: %s\n"%wrappername, outfile)
    child = pexpect.spawn(wrappername)
    try:
        result = 0
        for line in script:
            expecting = MakeList(line[0]) + [pexpect.EOF, pexpect.TIMEOUT]
            if line[1]:
                expecting = expecting + MakeList(line[1])
            dbprint ("expecting \"%s\"\n" % str(expecting), outfile)
            result = child.expect(expecting, timeout=5)
            dbprint ("child.before: " + child.before + "\n", outfile)
            dbprint ("\"%s\" Matched string: \"%s\"\n" % (str(expecting[result]), child.after), outfile)
            if result != 0:
                msg = "Got ERROR result from expect script: %s"%str(expecting[result]) 
                dbprint(msg + "\n" , outfile)
                return msg   
            dbprint ("Sending string \"%s\"\n" % line[2], outfile)
            child.sendline(line[2])
        if result == 0:
            dbprint("Waiting for process to give EOF\n")
            child.expect(pexpect.EOF, timeout=5)
    except:        
        dbprint("Caught error \"%s\"\n" % sys.exc_info()[0], outfile)
        errmsg = "ERROR running expect script"

    child.kill(signal.SIGKILL)
    dbprint("Child output remaining: \"%s\"\n"%str(child.before), outfile)
    child.close(False)
    return errmsg

# ================================================================
def run_test(test):
    global proc, gTestdir, gDatadir, proc
    proc = None
    errmsg = "SUCCESS"
        
    # ------------------------------------------------------------
    # Copy needed data into working directory
    os.chdir(gTestdir)
    test['need_data'] = MakeList(test['need_data'])
    for data in test['need_data']:
        need_data = "%s/%s"%(gTestdir,data)
        dbprint("need data: %s\n"% need_data)
        if  os.path.exists(need_data):
            dbprint("data exists\n")
        else:
            src_data = "%s/%s"%(gDatadir,data)
            if not os.path.exists(src_data):
                errmsg = "Error: Cannot find or copy needed data %s from %s"% (need_data,src_data)
            else:
                dbprint("copying %s to %s\n"%( src_data, need_data))
                if os.path.isdir(src_data):
                    shutil.copytree(src_data, need_data)
                else:
                    shutil.copy(src_data, need_data)
                dbprint("copied data to %s\n"% need_data)

    # ------------------------------------------------------------
    # Run the command
    test['success_pattern'] = MakeList(test['success_pattern'])
    outfilename = gTestdir+"%s.out"%test['name']
    fullcmd = "%s/%s %s"%(gBindir,test['cmd'],test['args'])
    outfile = open(outfilename, "w")
    outfile.close()
    outfile = open(outfilename, "r+")
    if errmsg == "SUCCESS":
        if 'pexpect' in test.keys():
            errmsg = RunTestExpect(fullcmd, test, outfile)
        else:
            errmsg = RunTestCommand(fullcmd, test, outfile)
    dbprint("Command completed; output saved in %s\n"% outfilename)

        
    # ------------------------------------------------------------
    # Check output of command
    if errmsg == "SUCCESS" and proc and proc.returncode and proc.returncode < 0:
        errmsg = "Command returned exit code %d."%proc.returncode

    test['output'] = MakeList(test['output'])
    if errmsg == "SUCCESS" and test['output']:
        for filename in test['output']:
            if not os.path.exists(filename):
                errmsg = "Output file %s was not created as expected.\n"%test['output']
                break

    if errmsg == "SUCCESS" and (test['success_pattern'] or test['failure_pattern']):
        success_patterns = MakeList(test['success_pattern'])    
        failure_patterns = MakeList(test['failure_pattern'])
        errmsg = CheckOutput(outfile, success_patterns, failure_patterns)

    # ------------------------------------------------------------
    # check return code of command if it failed
    returncode = 0
    if proc:
        returncode = proc.returncode
        if errmsg == "SUCCESS":
            if 'return' not in test.keys():
                test['return'] = 0
            if returncode != test['return']:
                errmsg = "FAILED: Process exited with undesirable return code %d"% returncode

    # ------------------------------------------------------------
    if errmsg == "SUCCESS":
        errmsg = FrameDiffs(test)
        
    
    # ------------------------------------------------------------
    # pretty things up a bit
    if errmsg != "SUCCESS":
        outfile.seek(0)
        output = outfile.read()
        if output:
            dbprint("Failure in command; detailed command output from file %s follows: \n"%outfilename)
            dbprint("\n"+"-"*50+"\n\n" )
            dbprint(output+'\n')
            dbprint("\n"+"-"*50+"\n\n" )
        outfile.write(errmsg+'\n')
    else:
        dbprint("\n"+"*"*50+"\n" )
        
    dbprint("%s\n"%errmsg)       
    dbprint("*"*80+"\n\n" )
    
    return  [errmsg == "SUCCESS", errmsg]

# ========================================================================
def RunTests(tests, stoponfail, create_gold_standard):

    if os.path.exists(gTestdir):
        shutil.rmtree(gTestdir)

    CreateDir(gTestdir)

    successes = 0
    results = []
    n = 0
    for test in tests:
        dbprint("\n"+ "="*80 +"\n" )
        dbprint("RUNNING TEST %d\n\n"%n)
        dbprint("CWD is %s\n"%os.getcwd())
        dbprint("TEST: %s\n\n"%str(test))
        dbprint("\n"+ "-"*40 +"\n" )
        test['create gold standard'] = create_gold_standard
        result = run_test(test)
        results.append(result)
        successes = successes + result[0]
        if stoponfail and not result[0]:
            dbprint("*"*50+"\n")
            dbprint("FAILED TEST %d, \"%s\", stopping per user preference.\n"%(n,test['name'])); 
            break
        n = n+1
        dbprint("\n"+ "="*80 +"\n" )

    dbprint("*"*50+"\n\n")
    if not stoponfail or  successes == len(tests):
        dbprint("successes:  %d out of %d tests\n"%(successes, len(tests)))
        dbprint("results: " + str(results) + "\n"+"*"*50+"\n")
        if successes != len(tests): 
            dbprint("Failed tests:\n")
            for n in range( len(tests)):
                if not results[n][0]:
                    dbprint("Test %d: %s: %s\n"%(n,tests[n]['name'], results[n][1]))
            dbprint("*"*50+"\n\n")
                
    dbprint("Results saved in %s\n\n"% str(gDBFilename))
    return [successes, results]

