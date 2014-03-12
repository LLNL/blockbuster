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
#include <QMutex>
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
QMutex registerProtector; 
map<QThread *, int> gThreadIdMap; 
int gMainThreadID = -1; 
/*static vector<QThread *> gThreads; 
  static vector<int> gThreadIDs; */
/*!
  Get the 0-based thread index for the given QThread. 
  Return -1 on failure.
*/
int GetThreadID(QThread *thread) {
  return gThreadIdMap[thread]; 
  /*  int i = 0; 
  int id = -1; 
  vector<QThread *>::iterator pos = gThreads.begin(), endpos = gThreads.end(); 
  while (pos != endpos && id == -1) {
    if (thread == *pos) {
      id = gThreadIDs[i];
    } else {
      ++pos; 
      ++i; 
    }
  }
  return id; */
}

/*!
  Once threads are registered, we can tell if we're in the main thread
*/
bool InMainThread(void) {
  return gThreadIdMap.empty() || gThreadIdMap[QThread::currentThread()] == gMainThreadID; 
}

/*!
  Call this from any thread to add it to the index of QThreads.  
*/
int RegisterThread(QThread *thread, int threadnum, bool isMain) {
  registerProtector.lock(); 
  gThreadIdMap[thread] = threadnum; 
  addMessageRec(thread, threadnum); 
  if (isMain) gMainThreadID = threadnum; 
  registerProtector.unlock(); 
  return threadnum; 
}

/*!
  This must also be done to keep things sane
*/
void UnregisterThread(QThread *thread) {
  registerProtector.lock(); 
  gThreadIdMap.erase(thread); 
  removeMessageRec(thread); 
  registerProtector.unlock(); 
  return ;
}
/*!
  Call this from any thread to get the current 0-based thread index.    
  Returns -1 if not found. 
*/
uint16_t GetCurrentThreadID(void) {
  registerProtector.lock(); 
  uint16_t id = GetThreadID(QThread::currentThread()); 
  registerProtector.unlock(); 
  return id; 
}

void PrintKeyboardControls(void)
{
  printf("\n=====================================================\n");
  printf("BLOCKBUSTER KEYBOARD CONTROLS\n");
  printf("Panning - press the left mouse button and drag the mouse to pan over the image.\n"); 
  printf("Zooming - press the middle mouse button and drag forward/backward to zoom in/out of the image.\n"); 
  printf("\n"); 
  printf("The following keyboard commands are recognized:\n"); 
  printf("\n"); 
  printf("Right/Left arrow - advance/backup by one frame.\n"); 
  printf("Shift + Right/Left arrow - advance/backup by 20 frames.\n"); 
  printf("Control + Right/Left arrow - advance/backup by 1/4 of movie length.\n"); 
  printf("Home - jump to the first frame, center the image and set zoom to one\n"); 
  printf("End - jump to the last frame in the movie\n"); 
  printf("Spacebar - play/pause\n"); 
  printf("c -   center the image in the window\n"); 
  printf("f -   zoom image to fit the window (minify only)\n"); 
  printf("m -   hide/unhide the mouse cursor over the movie window\n"); 
  printf("l/L - increase/decrease the current level of detail displayed\n"); 
  printf("q --  quit\n"); 
  printf("r -   play in reverse\n"); 
  printf("z/Z - zoom in or out\n"); 
  printf("1 -  (the number 1) set zoom to 1.0\n"); 
  printf("+/- - increase or decrease the frame rate\n"); 
  printf("i - display the GUI panel (interface) if it was hidden\n"); 
  printf("? or h -- print this menu\n"); 
  printf("Esc - exit Blockbuster\n"); 
  printf("=====================================================\n");

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


