#!/usr/bin/env python

import sys, os, shutil, time, threading, argparse, re, stat, pexpect, glob, Image, ImageChops
import signal
from subprocess import *

# =================================================================
gTestdir = None
gBindir = None
gDatadir = None
gDBFile = None
gDBFilename = None

# =================================================================
def SetBindir(dirname):
    global gBindir
    gBindir = dirname

# =================================================================
def SetDatadir(dirname):
    global gDatadir
    gDatadir = dirname
    
# =================================================================
def SetDBFile(outfilename=None):
    global gDBFilename, gDBFile, gTestdir
    if gDBFile:
        return
    
    if not gDBFilename:
        if not outfilename:
            outfilename = "%s/testing-results.out"%gTestdir                
        gDBFilename = outfilename
        dbprint("Debug output file is %s\n"%gDBFilename)

    gDBFile = open(gDBFilename, "w")
        
    return

# =================================================================
# Helper for FindPaths(). 
def FindBinDir(progname):
    bindir = None
    # try ../$SYS_TYPE/bin (usual case) and ../../$SYS_TYPE/bin (dev on LC)
    for dots in ['..','../..','../../..']:
        for subdir in [systype, '.']:
            trydir = "%s/%s/%s/bin"%(testdir,dots,subdir)
            # dbprint( "trying directory: %s\n"%trydir)
            if  os.path.exists(trydir+'/' + progname):
                return trydir
    return "%s/../%s/bin"%(testdir,systype)

# =================================================================
# Helper for FindPaths().  Do not call from elsewhere. 
def FindDataDir():
    datadir = os.path.abspath(os.path.dirname(sys.argv[0])+'/../sample-data')
    if not os.path.exists(datadir):
        proc = Popen(("tar -C %s -xzf %s.tgz"%(datadir.replace('sample-data',''), datadir)).split())
        proc.wait()
    if not os.path.exists(datadir):
        dbprint("WARNING: Cannot find or create data dir %s"%datadir)
        return None
    return datadir

# =================================================================
def FindPaths():
    global gBindir, gDatadir, gTestdir

    if gBindir and gDatadir and gTestdir:
        return

    # gTestdir
    if not gTestdir:
        gTestdir = "/tmp/"+os.getenv("USER")+"/blockbuster_tests/" 
        print "Creating new empty directory %s\n"%gTestdir
        if os.path.exists(gTestdir):
            shutil.rmtree(gTestdir)
        CreateDir(gTestdir)
            
    # gBindir
    if not gBindir:
        gBindir= FindBinDir('smtag')
        
    if not os.path.exists(gBindir):
        errexit("gBindir %s does not exist."%gBindir)
        # fix relative paths and stuff:
    if gBindir[-1] != '/':
        gBindir = gBindir + "/" 
    if gBindir[0] != '/':
        gBindir = os.getcwd() + '/' + gBindir
    binary = "%s/img2sm"%gBindir   
    gBindir = os.path.realpath(os.path.abspath(os.path.dirname(binary)))
    
    gDatadir = FindDataDir()
    if not gDatadir:
        errexit("Failed to create data directory!\n");
        
    dbprint( "gBindir  is: %s\n"%gBindir)
    dbprint( "gDatadir is: %s\n"%gDatadir)
    dbprint( "gTestdir is: %s\n"%gTestdir)
    
    
    return

# =================================================================
def dbprint(msg, outfile=None):
    SetDBFile()
    if outfile:
        outfile.write(msg)
    sys.stdout.write(msg)
    gDBFile.write(msg)
    
# =================================================================
def get_arg_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bindir', help="set directory where smtools live", default="")
    parser.add_argument('-o', '--output-dir', help="Where to put the test results", default="")
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
def CreateScript(cmd, filename, verbose=True, rawout=True):
    if os.path.exists(filename):
        os.remove(filename)
    script = open(filename, "w")
    script.write("#!/usr/bin/env bash \n");
    if verbose:
        script.write("set -xv\n");
        script.write("echo script running...\n");
    if rawout:
        script.write(cmd + ' >%s.rawout  2>&1 \n'%filename);
        script.write("savestat=$?\n")
        script.write("cat %s.rawout\n"%filename);
        script.write("exit $savestat\n")
    else:
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
    if "timeout"  in test.keys(): 
        timeout = int(test['timeout'])
    name = test['name']
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
# helper function for TagfileDiffs()
        # pattern to remove directory names from binaries:
        # JSON creates things like this:
        # '        "value": "\\/nfs\\/tmp2\\/rcook\\/blockbuster-copy\\/chaos_5_x86_64_ib\\/bin\\/img2sm steamboat\\/m00006.png steamboat\\/m00007.png steamboat\\/m00009.png --report -C -T steamtag:boats -E img2sm-canonical-tags.sm "'
        # we want to strip away build-specific paths:  
        # '        "value": "img2sm steamboat\\/m00006.png steamboat\\/m00007.png steamboat\\/m00009.png --report -C -T steamtag:boats -E img2sm-canonical-tags.sm "'
        # so here is the magic sauce:
        # searchexp = re.compile('\\\\[^ ]*/')
def RemoveBindirHostAndDateFromLine(line):
    r = re.search('\\\\[^ ]*/', line)
    if r:
        newline = line.replace(r.group(0), 'GENERIC-BINDIR')
        # dbprint("RemoveBindirFromLine: removing bindir from line.\n'%s' ------> '%s'\n"%(line,newline))
        line = newline

    r = re.search('Movie Create Host"\W*=\W*"(\w*)"', line)
    if r:
        newline = line.replace(r.group(1), 'GENERIC-HOST')
        # dbprint("RemoveBindirFromLine: removing bindir from line.\n'%s' ------> '%s'\n"%(line,newline))
        line = newline

    # Remove the date line, as for tests this changes every time
    # '\D' --> letters  '\d' --> digits
    r = re.search('.*("\D{3}[ ]+\D{3}[ ]+\d+[ ]+\d\d:\d\d:\d\d[ ]+\d{4}\s*\D{,2}T{,1}"\s*$)', line)
    if r:
        newline = line.replace(r.group(1), '"GENERIC-DATESTRING"')
        line = newline
        
    # Remove the Movie Creator line, as for tests this is not important
    # 
    r = re.search('ASCII.*Movie Creator"\W*=\W*"(.*)"', line)
    if r:
        newline = line.replace(r.group(1), '"GENERIC-CREATOR"')
        line = newline
        
    return line

# ================================================================
def TagfileDiffs(test):
    errmsg = "SUCCESS"
    if "tagfile diffs" not in test.keys() or not test['tagfile diffs']:
        return "SUCCESS"
    
    if type(test['tagfile diffs']) != type((2,)) and type(test['tagfile diffs']) != type([2]):
        test['tagfile diffs'] = [test['tagfile diffs']]

    for diff in test['tagfile diffs']:
        if diff[0] != '/':
            diff = "%s/%s"%(gTestdir, diff)
        dbprint("Diffing %s\n"%str(diff))

        standard = "%s.goldstandard"%diff
        try:
            tagfile = open(diff,'r')
        except:
            return "Cannot open tagfile %s"%diff;
        try:        
            stdfile = open(standard, 'r')
        except:
            return "Cannot open gold standard %s"%standard;
            
        dbprint("Diffing %s and %s\n"%(diff,standard))
        
        taglines = tagfile.readlines()
        stdlines = stdfile.readlines()
        if len(taglines) != len(stdlines):
            return "Different number of lines in %s than %s"%(diff, standard)
        inCreateHost = False
        for lineno in range(len(taglines)):         
            tagline = RemoveBindirHostAndDateFromLine(taglines[lineno])
            stdline = RemoveBindirHostAndDateFromLine(stdlines[lineno])
            # don't compare lines that report value for hosts, because that's different on each machine obviously and not a test failure
            if "Create Host" in tagline:
                inCreateHost = True
            if inCreateHost and '},' in tagline:
                inCreateHost = False
            if inCreateHost and '"value"' not in tagline:
                if tagline != stdline:
                    return "Mismatched fixed line between tagfile %s and standard %s at line %d:\ntagline: \"%s\"\n -->  \"%s\"\nstdline: \"%s\"\n -->  \"%s\""%(diff, standard, lineno, taglines[lineno], tagline, stdlines[lineno], stdline)

        return "SUCCESS"

# ================================================================
def FrameDiffs(test):
    errmsg = "SUCCESS"
    if "frame diffs" not in test.keys() or not test['frame diffs']:
        return "SUCCESS"
    # extract the frame
    dbprint("FrameDiffs: diff are \"%s\"\n"%str(test['frame diffs']));
    if type(test['frame diffs'][0]) != type((2,)) and type(test['frame diffs'][0]) != type([2,]):
        if len(test['frame diffs']) != 2:
            return "Bad frame diffs option: \"%s\""% str(test['frame diffs'])
        test['frame diffs'] = [[test['frame diffs'][0], test['frame diffs'][1]]]
    
    for diff in test['frame diffs']:
        dbprint("Diffing %s\n"%str(diff))
        movie = diff[0]
        frame = diff[1]
        if frame == -1:
            outframe = diff[0]
            if outframe[0] != '/':
                outframe = os.getcwd()+'/'+outframe
            standard = "%s/standards/%s"%(gDatadir, diff[0])
        else:
            outframe = "%s/%s_test_frame.png"%(os.getcwd(), test['name'])
            standard = "%s/standards/%s_standard_frame.png"%(gDatadir, test['name'])
            # Changed this to test smcat:
            # fullcmd = "%s/sm2img --first %d --last %d %s %s"%(gBindir, frame, frame, movie, outframe)
            fullcmd = "%s/smcat --first %d --last %d %s %s"%(gBindir, frame, frame, movie, outframe)
            outfilename = "%s.frame_diff.txt"%outframe
            dbprint("FrameDiffs: outfile is %s\n"%outfilename)
            outfile = open(outfilename, "w")
            outfile.close()
            outfile = open(outfilename, "r+")
            run_command(fullcmd.split(), outfile)
            errmsg = CheckOutput(outfile, "smcat successfully created frames", "ERROR")
            if errmsg != "SUCCESS":
                dbprint("ERROR: sm2img failed in FrameDiff(), output is in %s\n"%outfilename)
                return errmsg
        if test['create gold standard']:
            dbprint("Copying %s to %s to create new gold standard\n"%(outframe, standard))
            shutil.copy(outframe, standard)
            if not os.path.exists(standard):
                return "Could not copy %s to %s\n"%(outframe, standard)
        try:
            img = Image.open(outframe)
            gold = Image.open(standard)
            diff = ImageChops.difference(img,gold)
            for t in diff.getextrema():
                if t[0] or t[1]:
                    return "FrameDiffs(): Images %s and %s differ.  diff.getextrema: %s"%(outframe, standard, diff.getextrema())
        except:
            dbprint("Caught error \"%s\" with value \"%s\"\n" % (sys.exc_info()[0], sys.exc_info()[1]))
            return "FrameDiffs(): Got exception trying to diff images %s and %s."%(outframe, standard)
        
        dbprint ("FrameDiffs():  output image matches gold standard.\n")
    return 'SUCCESS'

# ================================================================
# for each line in the script, line[0] is the expect, line[1] are errors, and line[2] is the response
def RunTestExpect(fullcmd, test, outfile):
    errmsg =  "SUCCESS"
    wrappername = "%s/%s.sh"%(os.getcwd(), test['name'])
    CreateScript(fullcmd, wrappername, rawout=False)
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
    child.close()
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
                src_data = "%s/standards/%s"%(gDatadir,data)
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
        dbprint("Running command: %s\n"%fullcmd)
        dbprint("In working directory: %s\n"%os.getcwd())
        if 'pexpect' in test.keys():
            errmsg = RunTestExpect(fullcmd, test, outfile)
        else:
            errmsg = RunTestCommand(fullcmd, test, outfile)
        dbprint("Command completed; command output saved in %s\n"% outfilename)

        
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
    if errmsg == "SUCCESS":
        errmsg = TagfileDiffs(test)
        
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
    
    FindPaths()

    successes = 0
    results = []
    n = 0
    for test in tests:
        test['number'] = n
        dbprint("\n"+ "="*80 +"\n" )
        dbprint("RUNNING TEST %d: \"%s\"\n\n"%(n,test["name"]))
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

