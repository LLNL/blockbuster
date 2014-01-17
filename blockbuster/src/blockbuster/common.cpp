/* Copyright (c) 2003 Tungsten Graphics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files ("the
 * Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:  The above copyright notice, the Tungsten
 * Graphics splash screen, and this permission notice shall be included
 * in all copies or substantial portions of the Software.  THE SOFTWARE
 * IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "common.h"
#include <QTime>
#include <errmsg.h>
//bool gDbprintf= false; 
//static int gVerbose = 0; 

using namespace std; 
/* This file contains common routines that may be used by other
 * modules, both in sidecar and blockbuster.  
 */

/*!
  Global index of QThreads, used to get a simple 0-based threadID for each thread.  
*/ 
static vector<QThread *>gThreads; 
/*!
  Get the 0-based thread index for the given QThread. 
  Return -1 on failure.
*/
int GetThreadID(QThread *thread) {
  int i = 0; 
  vector<QThread *>::iterator pos = gThreads.begin(), endpos = gThreads.end(); 
  while (pos != endpos) {
    if (thread == *pos) return i;
    ++pos; 
    ++i; 
  }
  return -1; 
}

/*!
  Call this from any thread to add it to the index of QThreads.  
*/
void RegisterThread(QThread *thread) {
  if (GetThreadID(thread) == -1) {
    gThreads.push_back(thread); 
  }
  return; 
}

/*!
  This must also be done to keep things sane
*/
void UnregisterThread(QThread *thread) {
  vector<QThread *>::iterator pos = gThreads.begin(), endpos = gThreads.end(); 
  while (pos != endpos) {
    if (thread == *pos) {
      gThreads.erase(pos); 
      return; 
    }
    ++pos; 
  }
  return ;
}
/*!
  Call this from any thread to get the current 0-based thread index.    
  Returns -1 if not found. 
*/
uint16_t GetCurrentThreadID(void) { 
  return GetThreadID(QThread::currentThread()); 
}

void PrintKeyboardControls(void)
{
  dbprintf(0, "\n=====================================================\n");
  dbprintf(0, "BLOCKBUSTER KEYBOARD CONTROLS\n");
  dbprintf(0, "Panning - press the left mouse button and drag the mouse to pan over the image.\n"); 
  dbprintf(0, "Zooming - press the middle mouse button and drag forward/backward to zoom in/out of the image.\n"); 
  dbprintf(0, "\n"); 
  dbprintf(0, "The following keyboard commands are recognized:\n"); 
  dbprintf(0, "\n"); 
  dbprintf(0, "Right/Left arrow - advance/backup by one frame.\n"); 
  dbprintf(0, "Shift + Right/Left arrow - advance/backup by 20 frames.\n"); 
  dbprintf(0, "Control + Right/Left arrow - advance/backup by 1/4 of movie length.\n"); 
  dbprintf(0, "Home - jump to the first frame, center the image and set zoom to one\n"); 
  dbprintf(0, "End - jump to the last frame in the movie\n"); 
  dbprintf(0, "Spacebar - play/pause\n"); 
  dbprintf(0, "c -   center the image in the window\n"); 
  dbprintf(0, "f -   zoom image to fit the window (minify only)\n"); 
  dbprintf(0, "m -   hide/unhide the mouse cursor over the movie window\n"); 
  dbprintf(0, "l/L - increase/decrease the current level of detail displayed\n"); 
  dbprintf(0, "q --  quit\n"); 
  dbprintf(0, "r -   play in reverse\n"); 
  dbprintf(0, "z/Z - zoom in or out\n"); 
  dbprintf(0, "1 -  (the number 1) set zoom to 1.0\n"); 
  dbprintf(0, "+/- - increase or decrease the frame rate\n"); 
  dbprintf(0, "i - display the GUI panel (interface) if it was hidden\n"); 
  dbprintf(0, "? or h -- print this menu\n"); 
  dbprintf(0, "Esc - exit Blockbuster\n"); 
  dbprintf(0, "=====================================================\n");

  return;
}


//===============================================
/*
 * Return 1 if r1 contains r2, else return 0.
 */
int RectContainsRect(const Rectangle *r1, const Rectangle *r2)
{
   if (r1->x <= r2->x &&
       r1->y <= r2->y &&
       r1->x + r1->width >= r2->x + r2->width &&
       r1->y + r1->height >= r2->y + r2->height)
       return 1;
   else
       return 0;
}


/*
 * Return the union of two rectangles
 */
Rectangle RectUnionRect(const Rectangle *r1, const Rectangle *r2)
{
    Rectangle u;
    u.x = MIN2(r1->x, r2->x);
    u.y = MIN2(r1->y, r2->y);
    u.width = MAX2(r1->x + r1->width, r2->x + r2->width) - u.x;
    u.height = MAX2(r1->y + r1->height, r2->y + r2->height) - u.y;
	/*dbprintf(0," r1 %d %d %d %d  r2 %d %d %d %d\n",r1->x,r1->y,r1->width,r1->height,r2->x,r2->y,r2->width,r2->height);*/
    return u;
}



double GetCurrentTime(void)
{
   struct timeval tv;
   (void) gettimeofday(&tv, NULL);
   return (double) tv.tv_sec + (double) tv.tv_usec * 0.000001;
}


