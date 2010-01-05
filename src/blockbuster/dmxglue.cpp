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


#endif
