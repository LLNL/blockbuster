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
#ifdef USE_DMX
#include "MovieCues.h"
#include "SidecarServer.h"
#include "errmsg.h"
#include "xwindow.h"
#include "dmxglue.h"
#include "frames.h"
#include "util.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dmxRenderer.h"
dmxRenderer *gRenderer = NULL;


/*!
  This is set to be called at exit.  It looks at the last known
  gRenderer and deletes all the slaves that might be running.
*/ 
void
dmx_AtExitCleanup(void)
{
   ECHO_FUNCTION(5);
   if (!gRenderer || !gRenderer->numValidWindowInfos)
        return;

    int     slavenum = gRenderer->mActiveSlaves.size();

    while (slavenum--)
    {
        if (gRenderer->mActiveSlaves[slavenum])
            gRenderer->mActiveSlaves[slavenum]->
                SendMessage("Exit");
    }
    slavenum = gRenderer->mIdleSlaves.size();
    while (slavenum--)
    {
        gRenderer->mIdleSlaves[slavenum]->SendMessage("Exit");
    }
    return;
}



void dmx_Resize(Canvas *canvas, int newWidth, int newHeight, int cameFromX)
{
    

  ECHO_FUNCTION(5);
    bb_assert(canvas);
    bb_assert(newWidth >= 0);
    bb_assert(newHeight >= 0);

    // don't forget to update the main window for Movie Cue events:
    ResizeXWindow(canvas, newWidth, newHeight, cameFromX); 
    /* I think these are redundant, as ResizeXWindow sets them
       canvas->width = newWidth;
       canvas->height = newHeight;
    */ 
    if (gRenderer->haveDMX) {
        gRenderer->GetBackendInfo();
    }

    gRenderer->UpdateBackendCanvases();
}


void dmx_Move(Canvas *canvas, int newX, int newY, int cameFromX)
{
  /* OLD command (by BP?) 
     nothing, Resize() should also have been called in response to
     window move/resize and the DMX stuff will get updated there.
  */
  
  ECHO_FUNCTION(5);
  /* Rich Cook -- now that we can use sidecar to move a window, we need to 
     tell the backend servers to move the windows appropriately
  */ 
  MoveXWindow(canvas, newX, newY, cameFromX); 
  if (gRenderer->haveDMX) {
    gRenderer->GetBackendInfo();
  }
  
  gRenderer->UpdateBackendCanvases();
  
  /* I think these are redundant, as MoveXWindow sets them
     canvas->XPos = newX;
     canvas->YPos = newY;
  */ 
} 



/* XXX these should really be passed to the Preload() function */
/* Works OK to save them from the previous Render call though. */
static int PrevDestX = 0;
static int PrevDestY = 0;
static float PrevZoom = 1.0;

void dmx_Render(Canvas *, int frameNumber,
                const Rectangle *imageRegion,
                int destX, int destY, float zoom, int lod)
{
    
  ECHO_FUNCTION(5);
  if (!gRenderer->numValidWindowInfos) return; 

    int i;


    PrevDestX = destX;
    PrevDestY = destY;
    PrevZoom = zoom;

#if 0
    printf("DMX::Render %d, %d  %d x %d  at %d, %d  zoom=%f\n",
           imageRegion->x, imageRegion->y,
           imageRegion->width, imageRegion->height,
           destX, destY, zoom);
#endif

    /*
     * Loop over DMX back-end windows (tiles) computing the region of
     * the image to display in each tile, and the clipped image's position.
     * This is a bit tricky.
     */
    for (i = 0; i < gRenderer->numValidWindowInfos; i++) {
        const int scrn = gRenderer->dmxWindowInfos[i].screen;
#if 0
        if (gRenderer->handle[scrn]) {
            printf("  offset %d, %d\n",
                   gRenderer->dmxScreenInfos[scrn].xoffset,
                   gRenderer->dmxScreenInfos[scrn].yoffset);
            printf("  origin %d, %d\n",
                   gRenderer->dmxScreenInfos[scrn].xorigin,
                   gRenderer->dmxScreenInfos[scrn].yorigin);

            printf("Window %d:\n", i);
            printf("  screen: %d\n", gRenderer->dmxWindowInfos[i].screen);
            printf("  pos: %d, %d  %d x %d\n",
                   gRenderer->dmxWindowInfos[i].pos.x,
                   gRenderer->dmxWindowInfos[i].pos.y,
                   gRenderer->dmxWindowInfos[i].pos.width,
                   gRenderer->dmxWindowInfos[i].pos.height);
            printf("  vis: %d, %d  %d x %d\n",
                   gRenderer->dmxWindowInfos[i].vis.x,
                   gRenderer->dmxWindowInfos[i].vis.y,
                   gRenderer->dmxWindowInfos[i].vis.width,
                   gRenderer->dmxWindowInfos[i].vis.height);
        }
#endif
        if (gRenderer->dmxWindowInfos[i].window) {
            const XRectangle *vis = &gRenderer->dmxWindowInfos[i].vis;
            Rectangle newRegion;
            int newDestX, newDestY;
			
            gRenderer->ClipImageRegion(destX, destY, imageRegion, vis, zoom,
                                       &newDestX, &newDestY, &newRegion);
			
			gRenderer->mActiveSlaves[scrn]->SetCurrentFrame(frameNumber); 
            gRenderer->mActiveSlaves[scrn]->
			  SendMessage(QString("Render %1 %2 %3 %4 %5 %6 %7 %8 %9")
						  .arg(frameNumber)
						  .arg(newRegion.x) .arg(newRegion.y)
						  .arg(newRegion.width) .arg(newRegion.height)
						  .arg(newDestX) .arg(newDestY)
						  .arg(zoom).arg(lod));


        }
    }
}

void  dmx_DrawString(Canvas */*canvas*/, int row, int column, const char *str)
{
   ECHO_FUNCTION(5);
 if (!gRenderer->numValidWindowInfos) return; 
    
    int x = (column + 1) * gRenderer->fontHeight;
    int y = (row + 1) * gRenderer->fontHeight;
    int i;

    /* Send DrawString to back-end renderers, with appropriate offsets */
    for (i = 0; i < gRenderer->numValidWindowInfos; i++) {
        if (gRenderer->dmxWindowInfos[i].window) {
            const int scrn = gRenderer->dmxWindowInfos[i].screen;

            int tx = x - gRenderer->dmxWindowInfos[i].vis.x;
            int ty = y - gRenderer->dmxWindowInfos[i].vis.y;
            int tcol = tx / gRenderer->fontHeight - 1;
            int trow = ty / gRenderer->fontHeight - 1;

            gRenderer->mActiveSlaves[scrn]->
			  SendMessage(QString("DrawString %1 %2")
						  .arg(trow).arg(tcol));
			gRenderer->mActiveSlaves[scrn]->
			  SendMessage(QString(str));
        }
    }
}


void dmx_SwapBuffers(Canvas *canvas){
  ECHO_FUNCTION(5);
  static int32_t swapID = 0; 
  if (!gRenderer->numValidWindowInfos) return; 

  /* Only check the first slave and the hell with the rest , to ensure we stay roughly in sync */ 
  int numslaves = gRenderer->numValidWindowInfos;
  vector<bool> incomplete; 
  incomplete.assign(numslaves, true); 
 int numcomplete = 0; 
  uint32_t usecs = 0, lastusecs = 0; 
  DEBUGMSG("Checking for pending swaps"); 
  while (numcomplete < numslaves) {
    //gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents); 
    gCoreApp->processEvents(); 
    int slavenum = numslaves;
    while (slavenum--) {
      DMXSlave *theSlave = 
        gRenderer->
        mActiveSlaves[gRenderer->dmxWindowInfos[slavenum].screen];
      if (incomplete[slavenum]) {
        incomplete[slavenum] = !theSlave->CheckSwapComplete(swapID-1); 
        if (!incomplete[slavenum]) {
          DEBUGMSG("Marking slave %d complete", slavenum); 
          numcomplete ++; 
        }
      }
      if (usecs - lastusecs > 10000) {
        DEBUGMSG("still checking swaps after %d usecs", usecs); 
        lastusecs = usecs;
      }
    }
    if (usecs > 10*1000*1000) {
      cerr << "Something is wrong. Slave has not swapped buffers after 10 seconds." << endl;
      if (gSidecarServer) {
        MovieEvent event(MOVIE_STOP_ERROR);
        gSidecarServer->AddEvent(event);
      }
      return; 
    }
    usecs += 500; 
    usleep (500);     
  }  
  
  uint16_t slaveNum = 0; 
  for (slaveNum=0; slaveNum< gRenderer->mActiveSlaves.size(); slaveNum++ ) {
    /* Not all slaves have valid buffers to swap, but they will figure this out, and if MPI is involved, it's important they all get to call MPI_Barrer(), so send swapbuffers to everybody */    
	gRenderer->mActiveSlaves[slaveNum]->SwapBuffers(swapID, canvas->playDirection, canvas->preloadFrames, canvas->startFrame, canvas->endFrame);
  }
  swapID ++; 
  return; 
}
 


#endif
