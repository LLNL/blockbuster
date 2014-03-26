#!/usr/bin/env python
import sys, re
startTime = endTime = None
previousEnd = None
frame=0
oldframe = 0
loop = 0
times={}
totalrendertime = 0.0
totalinterframe = 0.0
totaltime = 0.0
frames = 0

highframetime = 20
highinterframe = 10

if len(sys.argv) < 2:
    print "Error:  need a logfile name"
    sys.exit(1)

files = []
for arg in sys.argv[1:]:
    if arg[0] == '-':
        if arg == '--stereo' or arg == '-s':
            highframetime = 40
        else:
            errexit("Bad argument: %s"%arg)
    else:
        files.append(arg)

frameRate = -1.0

frameratesearch = re.compile("mFrameRate=([0-9]+\.[0-9]+)")
for infile in files:
    if '.log' in infile:
        outfilename = infile.replace('.log', '-analysis.log')
    else:
        outfilename = infile + '-analysis.log'
    print "writing results for file %s to file: %s"%(infile,outfilename)
    outfile = open(outfilename, "w")
    for line in open(infile, "r"):
        exp = frameratesearch.search(line)
        if exp:
            framerate = float(exp.group(0).split("=")[1])
            # outfile.write("Found frame rate %f in line: %s"%(framerate, line))
        if '"gl_stereo"' in line:
            outfile.write("stereo renderer detected, using higher frame time\n")
            highframetime = 40
        if '"gl"' in line:
            outfile.write("gl renderer detected, using lower frame time\n")
            highframetime = 20            
        if "RenderActual begin" in line:
            tokens=line.split()
            try:
                frame=int(tokens[6].strip(',').strip(':'))
                startTime = float(tokens[len(tokens)-1].split("=")[1].strip(']'))
                if frame < oldframe:
                    loop = loop + 1
            except:
                startTime=None
                outfile.write( "warning: bad 'start time' line at frame %d: \"%s\"\n" %( frame, line))
                    
        elif "RenderActual end" in line:
            if not startTime:
                outfile.write( "Warning: skipping end time analysis at frame %d\n"%frame)
                continue
            tokens=line.split()
            try:
                endTime = float(tokens[10].split("=")[1].strip(']'))
                if previousEnd:
                    interframe = 1000.0*(startTime - previousEnd)
                else:
                    interframe = 0
                    
                previousEnd = endTime
                rendertime = 1000.0*(endTime - startTime)
                
                entry = {"frame": frame,
                         "frame rate": framerate, 
                         "frame time": rendertime,
                         "interframe time": interframe,
                         "total frame time": rendertime+interframe,
                         "start": startTime,
                         "end": endTime }
                totalrendertime = totalrendertime + rendertime
                totalinterframe = totalinterframe +  interframe
                totaltime = totaltime + rendertime+interframe
                frames = frames + 1
                if loop not in times.keys():
                    # outfile.write( "Adding new dictionary to times\n")
                    times[loop] = {}
                    # outfile.write( "Adding new entry to loop %s: %s\n"%(loop, str(entry)))
                times[loop][frame] = entry
            except:
                endTime=None
                outfile.write( "warning: bad 'end time' line at frame %d: \"%s\"\n" %( frame, line))
                
                continue

    outfile.write( "Total frames: %d\n"%frames)
    outfile.write( "Average frame time: %f\n"%(totaltime/frames))
    outfile.write( "Average render time: %f\n"%(totalrendertime/frames))
    outfile.write( "Average interframe time: %f\n"%(totalinterframe/frames))

    # outfile.write( "times: %s\n"%str(times))

    loops = times.keys()
    loops.sort()
    for loop in loops:
        outfile.write( "loop: %s\n"%loop)
        keys = times[loop].keys()
        keys.sort()
        for time in keys:
            if times[loop][time]["frame time"] > highframetime:
                outfile.write( "High frame time! ********************************\n")
            if times[loop][time]["interframe time"] > highinterframe:
                outfile.write( "High interframe time! ********************************\n")
            if times[loop][time]["total frame time"] > highframetime + highinterframe:
                outfile.write( "High total frame time! ********************************\n")
            outfile.write( str(times[loop][time]) + "\n")
