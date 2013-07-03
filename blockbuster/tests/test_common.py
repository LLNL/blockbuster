#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, re, stat
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
                outfilename = os.getcwd() + "/" + "results.out"
            gDBFilename = outfilename
            dbprint("Debug output file is %s\n"%gDBFilename)
        gDBFile = open(gDBFilename, "w")
    return

# =================================================================
def dbprint(msg):
    SetDBFile()
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
def CreateScript(cmd, filename):
    if os.path.exists(filename):
        os.remove(filename)
    script = open(filename, "w")
    script.write("#!/usr/bin/env bash \n");
    script.write("set -xv\n");
    script.write("echo script running...\n");
    script.write(cmd + ' 2>&1 \n');
    script.close()
    os.chmod(filename, stat.S_IRUSR | stat.S_IXUSR)
    return

# =================================================================
proc = None
def run_command(cmd, outfile):
    global proc
    dbprint( "Running command: \"%s\"\n"%cmd)
    proc = Popen(cmd, bufsize=-1, stdout=outfile, stderr=STDOUT)
    proc.wait()
    return 

# =================================================================
# python 2.7 has check_output, but we are assuming 2.6 here
def CheckOutput(cmd):
    outfile = tempfile.NamedTemporaryFile()
    run_command(cmd, outfile)
    outfile.seek(0)
    return outfile.read()

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
def run_test(test, timeout=15):
    global proc, gTestdir, gDatadir
    errmsg = "SUCCESS"
    if not os.path.exists(gTestdir):
        CreateDir(gTestdir)
    os.chdir(gTestdir)
    dbprint("\n"+ "="*80 +"\n"+ "="*80 +"\n\n" )
    dbprint("run_test, cwd is %s, running test: %s\n"%(os.getcwd(), str(test)))
    if test['need_data']:
        need_data = "%s/%s"%(gTestdir,test['need_data'])
        dbprint("need data: %s\n"% need_data)
        if  os.path.exists(need_data):
            dbprint("data exists\n")
        else:
            src_data = gDatadir+'/'+need_data
            if not os.path.exists(src_data):
                errmsg = "Error: Cannot find or copy needed data %s"% need_data
            else:
                dest = gTestdir+"/"+need_data
                dbprint("copying %s to %d\n"%( src_data, dest))
                if os.path.isdir(src_data):
                    shutil.copytree(src_data, dest)
                else:
                    shutil.copy(src_data, dest)
                dbprint("copied data to %s\n"% dest)

    success_patterns = MakeCompiledList(test['success_pattern'])    
    failure_patterns = MakeCompiledList(test['failure_pattern'])
    outfilename = gTestdir+"%s.out"%test['name']
    fullcmd = "%s/%s %s"%(gBindir,test['cmd'],test['args'])
    outfile = open(outfilename, "w")
    outfile.close()
    outfile = open(outfilename, "r+")
    if errmsg == "SUCCESS":
        outfile.write(fullcmd+'\n')
        outfile.write("Working directory: %s\n"%os.getcwd())
        outfile.flush()
        
        scriptname = "%s/%s.sh"%(os.getcwd(), test['name'])
        CreateScript(fullcmd, scriptname)
        dbprint("Full command is \"%s\", placed in script %s\n"%(fullcmd, scriptname))
        
        theThread = threading.Thread(target=run_command, args=([scriptname, outfile]))
        theThread.start()
        dbprint("Waiting %d seconds for thread to finish...\n"%timeout)
        theThread.join(timeout)
        if theThread.isAlive():
            os.kill(proc.pid,9)
            errmsg = "ERROR: Command failed to exit within timeout %d seconds!\n"%timeout
        dbprint("command output saved in %s\n"% outfilename)
    
        
    if errmsg == "SUCCESS" and proc and proc.returncode and proc.returncode < 0:
        errmsg = "Command returned exit code %d."%proc.returncode
        
    if errmsg == "SUCCESS" and test['output'] and not os.path.exists(test['output']):
        errmsg = "Output file %s was not created as expected.\n"%test['output']

    if errmsg == "SUCCESS" and test['success_pattern'] or test['failure_pattern']:
        outfile.seek(0)
        found_failure = False
        found_success = False
        while not found_success and not found_failure:
            line = outfile.readline()
            if not line:
                break
            for pattern in success_patterns:
                if re.search(pattern, line):
                    found_success=True
            for pattern in failure_patterns:
                if re.search(pattern, line):
                    found_failure=True                
        if not found_success:
            errmsg = "Expected success pattern \"%s\" not found in output."%str(test['success_pattern'])
        if found_failure:
            errmsg = "Found failure pattern \"%s\" in output."%str(test['failure_pattern'])
            
    if errmsg == "SUCCESS":
        resultstring = errmsg
    else:
        returncode = None
        if proc:
            returncode = proc.returncode
        result = [False, errmsg]
        resultstring = "FAILED.  Return code %s, reason: \"%s\"\n"%(str(returncode),errmsg)
        
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
        
    dbprint("%s\n"%resultstring)       
    dbprint("*"*80+"\n\n" )
    
    return  [errmsg == "SUCCESS", errmsg]

# ========================================================================
def RunTests(tests):

    successes = 0
    results = []
    for test in tests:
        result = run_test(test)
        results.append(result)
        successes = successes + result[0]

    resultstring = "*"*50+"\n" + "successes:  %d out of %d tests\n"%(successes, len(tests)) + "results: " + str(results) + "\n"+"*"*50+"\n"
    dbprint(resultstring)
    dbprint("Results saved in %s\n"% str(gDBFilename))
    return [successes, results]

