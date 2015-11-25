LLNL Note:  Be sure to test against actual cuefiles on both OCF and SCF, as some bugs do crop up on SCF that don't appear on OCF.  
2015-11-25 Python 2.7.10, the default, does not have zip support and this blows up the tests.  Until this is fixed, you must do this:

use python-2.7.7
make test

==============================================================
# Be strict in the test procedure!  Document bugs during testing. 

Before testing, go to the Blockbuster sourceforge site and open the bug tracker.  You will need it.  :-)  Don't bother with private notes if you can avoid it.  The tracker is there for a reason;  use it!  

Similarly, do not fix bugs while testing, unless they are catastrophic and testing or installation cannot continue without them.   Do not stop to fix a little bug here and there.  To do so diffuses focus and results in testing not getting completed.  Also, the bug you rush off to fix is  not any more important as others that are waiting to be fixed.  Just document it and continue.

Capture bugs in the testing system located in the tests/ subdirectory of the source.  

Also, a consistent need to deviate from the test procedure could be indicative of a bug in the test procedure itself.  Document that here and continue!  

============================================================
SHORT BUT "GOOD" RELEASE TESTING:  

Before doing this, run "make test" to make sure no regressions have cropped up.  

A.  Duplicate Scott Miller's work flow: 
1.  Open sidecar with a cuefile as an argument.  
2.  Create a new cue with a movie name in it.  
3.  Execute the cue so blockbuster runs on the wall.  
4.  Size and position the movie to taste, moving to the right frame number and setting up stereo. 
5.  Take a snapshot and save the cue. 
6.  Execute the cue to make sure it works.  

B.  Run through a cuefile: 
1.  Open sidecar with a cuefile as argument. 
2.  Run all the cues in the file.  Watch performance and behavior.  Make sure to observe change between fullscreen/nofullscreen and stereo/mono modes.  Screw with blockbuster, try to go to extremes of wiggling, zooming, panning.  
3.  Change one cue.  Save the file.  Reopen it.  

C.  Exercise blockbuster: 
1.  Open a movie in blockbuster from the command line, using various arguments.  
2.  As in B above, screw with blockbuster, try to go to extremes of wiggling, zooming, panning.  

LONG AND STRICT VERSION: 
Start with "SHORT" tests from above, then test:  

==============================================================
# New Features and Fields in the GUI
If any new fields or features have been added, test to see if the defaults are set up correctly and handled well in blockbuster and sidecar.  Also check if unusual values are used, do they work?  Frame rate was a great example.  FPS = 0.0 caused very bad things to happen.  What happens if you type a word or a negative number for the frame rate?  Etc.  

==============================================================
# Cuefiles 
Use Testing.cues for a good suite of tests.  
Run sidecar without loading a cuefile.  Next, load a cuefile both from the command line and from the "Load from file..." button.  Save it as a new cue file and then reload it. Run all cues.  Test every button in the interface that operates on cues.  Duplicate a cue, then test every button and checkbox in the new cue.  Try to close the window and quit the application to make sure it prompts for saves. Save the cue file, then reload it.  Make sure they were saved and restored properly.  

==============================================================
# Movie Cues and Snapshots
Execute a cue and create a new server in the resulting dialog.  Create a new cue, adjust the zoom, position and size of the window and of the image and take a snapshot of the result.  Click "Apply Changes" and then execute the cue again and make sure it is the same as it was.  
Select multiple cues and execute them all with the "Execute" button.  Interrupt in the middle once.  Now execute them with the "Loop" button.  Also interrupt that.  
Be sure to test a "powerpoint slideshow" type movie in there somewhere.  

==============================================================
# Remote Control:
Test -movie and -play options to sidecar. 
The "-movie movie@host" flags work, but what is the actual use case for that?  In my tests, either DISPLAY is not set correctly or else the wrong blockbuster launches, so it's pretty useless at LLNL. Why did I make this possible?  
Check every button in the remote control interface to make sure it works correctly.  Enable and disable keystroke capture.   Test the following keystrokes: 
Right/Left arrow - advance/backup by one frame.
Shift + Right/Left arrow - advance/backup by 20 frames.
Control + Right/Left arrow - advance/backup by 1/4 of movie length.
Home - jump to the first frame
End - jump to the last frame in the movie
Spacebar - play/pause
c -   center the image in the window
f -   zoom image to fit the window (minify only)
m -   hide/unhide the mouse cursor over the movie window
l/L - increase/decrease the current level of detail displayed
q --  quit
r -   play in reverse
z/Z - zoom in or out
1 -  (the number 1) set zoom to 1.0
+/- - increase or decrease the frame rate
i - display the GUI panel (interface) if it was hidden
? or h -- print this menu
Esc - exit Blockbuster


Take a screen capture using the "Save Image" button. 

Confirm proper looping behaviors, and pingpong. 
Confirm that the file history is working in the blockbuster launch dialog.

==============================================================
# connections:
Repeatedly connect and disconnect from the server, then test the controls.  Terminate it and restart it.  Execute cues with a connection in place and out of place.  Do this several times and make sure the right buttons are enabled and disabled and all functionality is available.  Connect with cues shown and hidden.  Quit blockbuster from within blockbuster and make sure sidecar handles that. 

==============================================================
# Performance:
Check rendering speed under MPI and without MPI and confirm that synchronization is occurring reliably.  

==============================================================
# DMX 
Test launch and behavior of blockbuster and sidecar with and without DMX and MPI framesync. Test "Fill" (fullscreen) button with DMX. Ensure a cue or two works properly in DMX. 

==============================================================
# Stereo 
Test a stereo movie out with DMX and confirm how nice it all is. 

==============================================================
# SM tools
If any changes were made in the sm or smapps libraries, or might affect them, then run a few tools from the smlib to test them too.  Especially smcat, sm2img, img2sm.  

==============================================================
# Debug vs. Production modes
Before installing, but after confirming functionality with the below tests, build the development version with debug turned off and test to make sure it's quiet.  


