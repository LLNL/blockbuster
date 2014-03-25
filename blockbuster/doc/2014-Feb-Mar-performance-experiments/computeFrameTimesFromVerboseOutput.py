#!/usr/bin/env python
import sys
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


if len(sys.argv) < 2:
    print "Error:  need a logfile name"
    sys.exit(1)
    
for line in open(sys.argv[1], "r"):    
    if "RenderActual begin" in line:
        tokens=line.split()
        try:
            frame=int(tokens[6].strip(',').strip(':'))
            startTime = float(tokens[len(tokens)-1].split("=")[1].strip(']'))
            if frame < oldframe:
                loop = loop + 1
        except:            
            startTime=None
            print "warning: bad 'start time' line at frame %d: \"%s\"" %( frame, line)

    elif "RenderActual end" in line:
        if not startTime:
            print "Warning: skipping end time analysis at frame %d"%frame
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

            entry = {"frame time": rendertime,
                     "interframe time": interframe,
                     "total frame time": rendertime+interframe, 
                     "start": startTime,
                     "end": endTime }
            totalrendertime = totalrendertime + rendertime
            totalinterframe = totalinterframe +  interframe
            totaltime = totaltime + rendertime+interframe
            frames = frames + 1
            if loop not in times.keys():
                #print "Adding new dictionary to times"
                times[loop] = {}
            #print "Adding new entry to loop %s: %s"%(loop, str(entry))
            times[loop][frame] = entry
        except:
            endTime=None
            print "warning: bad 'end time' line at frame %d: \"%s\"" %( frame, line)
            
            continue

print "Total frames:", frames
print "Average frame time:", totaltime/frames
print "Average render time:", totalrendertime/frames
print "Average interframe time:", totalinterframe/frames

#print "times:", times

loops = times.keys()
loops.sort()
for loop in loops:
    print "loop:", loop
    keys = times[loop].keys()
    keys.sort()
    for time in keys:
        if times[loop][time]["frame time"] > 20:
            print "High frame time! ********************************"
        if times[loop][time]["interframe time"] > 10:
            print "High interframe time! ********************************"
        if times[loop][time]["total frame time"] > 30:
            print "High total frame time! ********************************"
        print times[loop][time]
        
