2012-04-13
Threading is now the major obstacle to blockbuster performance.  I think the easiest way to proceed is a major rewrite, because the code is very creaky. 

My 8 week plan to rule the world.  7 Steps to rewrite blockbuster.

1)  write my own threadsafe getframe() function that can retrieve an image of any format and put it into a frame class (2 weeks)  DONE

2) display the frame into a window (1 week)  DONE

3) play a sequence of frames into the window and get timings for large movies. (1 week) 
   For langerGZ.sm, with blockbuster, I sometimes get 100-150 FPS using 85% of a CPU, and sometimes get 60 FPS using 45% of a CPU.  Could be thread timing issues?  
   OK -- the first pass completely serial, I get  51.6134 FPS using 85% of a CPU. Hmm WTF? 
   I notice that the fully buffered speed is 140 FPS, slowing to perhaps 130 if I add a crop() operation before each display();  So I can use crop and not recompute subimages when displaying movies.  

4) make #4 threaded (1 week)

5) Clean up the code from 1-4 and take a breath. 

6) drop in the blockbuster GUI with "stop and play" functionality enabled (1 week)

7) Implement the rest of the GUI (1 week)

8) Add sidecar functionality (1 week)



2011-03-04
Rich Cook
Let's make this library more modern.  
The old library did not allow buffered I/O, a major flaw, and was complicated and not thread-safe.  The new design will allow all these things. 

