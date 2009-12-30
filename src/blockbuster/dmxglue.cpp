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

//============================================================

MovieStatus dmx_Initialize(Canvas *canvas, const ProgramOptions *options) {
  DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;
  uint16_t i;
  ECHO_FUNCTION(5);

  gRenderer = dynamic_cast<dmxRenderer *>(options->mNewRenderer); 
  
  if(options->backendRendererName != "") {
    gRenderer->mBackendRenderer = options->backendRendererName;
    QString msg("User specified renderer %1 for the backend\n"); 
    cerr << msg.toStdString(); 
    INFO(msg.arg(gRenderer->mBackendRenderer));
  }
  else {
    QString msg("No user specified backend renderer.  Using default.\n");
    INFO(msg);
  }
  
  /* Plug in our special functions for canvas manipulations.
   * We basically override all the functions set in CreateXWindow.
   */
  canvas->SetFrameListPtr = dmx_SetFrameList;
  canvas->RenderPtr = dmx_Render;
  canvas->ResizePtr = dmx_Resize;
  canvas->MovePtr = dmx_Move;
  canvas->DestroyRendererPtr = dmx_DestroyRenderer;
  canvas->SwapBuffersPtr = dmx_SwapBuffers;
  /* If the UserInterface implements this routine, we should not use ours */
  if (canvas->DrawStringPtr == NULL) { 
    canvas->DrawStringPtr = dmx_DrawString;
  }
  
  /* Get DMX info */
  if (IsDMXDisplay(glueInfo->display)) {
    /* This will reset many of the values in gRenderer */
    GetBackendInfo(canvas);
  }
  else {
#ifdef FAKE_DMX
    FakeBackendInfo(canvas);
#else
    ERROR("'%s' is not a DMX display, exiting.",
          DisplayString(glueInfo->display));
    exit(1);
#endif
  }
  for (i = 0; i < gRenderer->dmxScreenInfos.size(); i++) {
    QHostInfo info = QHostInfo::fromName(QString(gRenderer->dmxScreenInfos[i]->displayName).split(":")[0]);
    QHostAddress address = info.addresses().first();
    DEBUGMSG(QString("initializeing display name from %1 to %2 with result %3").arg(gRenderer->dmxScreenInfos[i]->displayName).arg(info.hostName()).arg(address.toString())); 
    gRenderer->dmxHostAddresses.push_back(address); 
    DEBUGMSG(QString("put on stack as %1").arg(gRenderer->dmxHostAddresses[i].toString())); 
  }
  /* Get a socket connection for each back-end instance of the player.
     For each dmxScreenInfo, launch one slave and create one QHostInfo from the name.
     Note that the slave will not generally match the HostInfo... we don't know
     what order the slaves will connect back to us.  That's in fact the point of 
     creating the QHostInfo in the first place.
  */ 
  DEBUGMSG("Launching slaves..."); 
  
  gRenderer->setNumDMXDisplays(gRenderer->dmxScreenInfos.size());
  for (i = 0; i < gRenderer->dmxScreenInfos.size(); i++) {
    
    if (i==0 || options->slaveLaunchMethod != "mpi") {
      QString host(gRenderer->dmxScreenInfos[i]->displayName);
      /* remove :x.y suffix */
      if (host.contains(":")) {
        host.remove(host.indexOf(":"), 100); 
      }
      
      gRenderer->LaunchSlave(host);
    }
  }
  
  /*!
    Wait for all slaves to phone home
  */ 
  uint64_t msecs = 0;
  while (!gRenderer->slavesReady() && msecs < 30000) {// 30 secs
    //gCoreApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    gCoreApp->processEvents();
    gRenderer->QueueSlaveMessages(); 
    msecs += 10; 
    usleep(10000); 
  }
  if (!gRenderer->slavesReady()) {
    ERROR("Slaves not responding after 30 seconds");
    return MovieFailure; 
  }   
  
  UpdateBackendCanvases(canvas);
  
  return MovieSuccess; /* OK */
}


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


/*!
  just a block of code that gets used a few times when there is an error: 
*/ 
void ClearScreenInfos(void) {
  uint32_t i=0; 
  while (i < gRenderer->dmxScreenInfos.size()) delete gRenderer->dmxScreenInfos[i]; 
  gRenderer->dmxScreenInfos.clear(); 
  if (gRenderer->dmxWindowInfos) delete [] gRenderer->dmxWindowInfos; 
  return; 
}

/*
 * Get the back-end window information for the given window on a DMX display.
 */
void GetBackendInfo(Canvas *canvas)
{
  DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;
  
  int i, numScreens; 
  gRenderer->haveDMX = 0;
    
  DMXGetScreenCount(glueInfo->display, &numScreens);
  if ((uint32_t)numScreens != gRenderer->dmxScreenInfos.size()) {
	ClearScreenInfos(); 
	for (i = 0; i < numScreens; i++) {
	  DMXScreenInfo *newScreenInfo = new DMXScreenInfo; 
	  gRenderer->dmxScreenInfos.push_back(newScreenInfo); 
	}
	gRenderer->dmxWindowInfos = new DMXWindowInfo[numScreens]; 
  }
  for (i = 0; i < numScreens; i++) {
	if (!DMXGetScreenInfo(glueInfo->display, i, gRenderer->dmxScreenInfos[i])) {
	  ERROR("Could not get screen information for screen %d\n", i);
	  ClearScreenInfos(); 
	  return;
	}
  }
  

  /*!
	Ask DMX how many screens our window is overlapping and by how much.
	There is one windowInfo info for each screen our window overlaps 
   */ 
  if (!DMXGetWindowInfo(glueInfo->display,
						glueInfo->window, &gRenderer->numValidWindowInfos,
						numScreens,
						gRenderer->dmxWindowInfos)) {
	ERROR("Could not get window information for 0x%x\n",
		  (int) glueInfo->window);
	ClearScreenInfos(); 
	return;
  }
  uint16_t winNum = 0; 
  for (winNum=gRenderer->numValidWindowInfos; winNum < gRenderer->dmxScreenInfos.size(); winNum++) {
	gRenderer->dmxWindowInfos[winNum].window = 0;	
  }
  gRenderer->haveDMX = 1;
}

#ifdef FAKE_DMX

void FakeBackendInfo(Canvas *canvas)
{
    

    /* two screens with the window stradling the boundary */
    const int screenWidth = 1280, screenHeight = 1024;
    const int w = 1024;
    const int h = 768;
    const int x = screenWidth - w / 2;
    const int y = 100;
    int i;

    gRenderer->haveDMX = 0;

    gRenderer->numScreens = 2;
    gRenderer->numWindows = 2;


#if DMX_API_VERSION == 1
    gRenderer->dmxScreenInfo = (DMXScreenInformation *)
        calloc(1, gRenderer->numScreens * sizeof(DMXScreenInformation));
#else
    gRenderer->dmxScreenInfo = (DMXScreenAttributes *)
        calloc(1, gRenderer->numScreens * sizeof(DMXScreenAttributes));
#endif

    if (!gRenderer->dmxScreenInfo) {
        ERROR("FakeBackendDMXWindowInfo failed!\n");
        gRenderer->numScreens = 0;
        return;
    }


#if DMX_API_VERSION == 1
    gRenderer->dmxWindowInfo = (DMXWindowInformation *)
        calloc(1, gRenderer->numScreens * sizeof(DMXWindowInformation));
#else
    gRenderer->dmxWindowInfo = (DMXWindowAttributes *)
        calloc(1, gRenderer->numScreens * sizeof(DMXWindowAttributes));
#endif

    if (!gRenderer->dmxWindowInfo) {
        ERROR("Out of memory in FakeBackendDMXWindowInfo\n");
        free(gRenderer->dmxScreenInfo);
        gRenderer->dmxScreenInfo = NULL;
        gRenderer->numScreens = 0;
        return;
    }

    for (i = 0; i < gRenderer->numScreens; i++) {
       gRenderer->dmxScreenInfo[i].displayName = strdup("localhost:0");

       gRenderer->dmxScreenInfo[i].logicalScreen = 0;
#if DMX_API_VERSION == 1
       gRenderer->dmxScreenInfo[i].width = screenWidth;
       gRenderer->dmxScreenInfo[i].height = screenHeight;
       gRenderer->dmxScreenInfo[i].xoffset = 0;
       gRenderer->dmxScreenInfo[i].yoffset = 0;
       gRenderer->dmxScreenInfo[i].xorigin = i * screenWidth;
       gRenderer->dmxScreenInfo[i].yorigin = 0;
#else
       /* xxx untested */
       gRenderer->dmxScreenInfo[i].screenWindowWidth = screenWidth;
       gRenderer->dmxScreenInfo[i].screenWindowHeight = screenHeight;
       gRenderer->dmxScreenInfo[i].screenWindowXoffset = 0;
       gRenderer->dmxScreenInfo[i].screenWindowYoffset = 0;
       gRenderer->dmxScreenInfo[i].rootWindowXorigin = i * screenWidth;
       gRenderer->dmxScreenInfo[i].rootWindowYorigin = 0;
#endif

       gRenderer->dmxWindowInfo[i].screen = i;
       gRenderer->dmxWindowInfo[i].window = 0;

#if DMX_API_VERSION == 1
       gRenderer->dmxWindowInfo[i].pos.x = x - gRenderer->dmxScreenInfo[i].xorigin;
       gRenderer->dmxWindowInfo[i].pos.y = y - gRenderer->dmxScreenInfo[i].yorigin;
#else
       gRenderer->dmxWindowInfo[i].pos.x = x - gRenderer->dmxScreenInfo[i].rootWindowXorigin;
       gRenderer->dmxWindowInfo[i].pos.y = y - gRenderer->dmxScreenInfo[i].rootWindowYorigin;
#endif

       gRenderer->dmxWindowInfo[i].pos.width = w;
       gRenderer->dmxWindowInfo[i].pos.height = h;
       gRenderer->dmxWindowInfo[i].vis.x = i * w / 2;
       gRenderer->dmxWindowInfo[i].vis.y = 0;
       gRenderer->dmxWindowInfo[i].vis.width = w / 2;
       gRenderer->dmxWindowInfo[i].vis.height = h;
    }
}
#endif

/*
 * Check if display is on a DMX server.  Return 1 if true, 0 if false.
 */
int IsDMXDisplay(Display *dpy)
{
  ECHO_FUNCTION(5);
   Bool b;
   int major, event, error;
   b = XQueryExtension(dpy, "DMX", &major, &event, &error);
   return (int) b;
 }


/* This SetFrameList method doesn't save a local framelist.
 */
 void dmx_SetFrameList(Canvas *canvas, FrameList *frameList){
    
  ECHO_FUNCTION(5);
    uint32_t framenum;
	uint32_t i; 
	QString previousName; 
	
    canvas->frameList = frameList;
	
    /* concatenate filenames.  We want to send only unique filenames 
     * back down to the slaves (or they'd try to load lots of files);
     * we cheat a little here and just send a filename if it's not
     * the same as the previous filename.
     *
     * Although this is not quite proper (it's allowable in the design
     * for the main program to rearrange frames so that frames from
     * different files are interleaved), it works for now.
     *
     * A more complex (but technically correct) solution would be to send
     * the list of frames as a list of {filename, frame number} pairs,
     * which is how the main program references frames.  But this would
     * require a lot more complexity and no more utility, since there's
     * no way the main program can take advange of such a feature now.
     */
    gRenderer->files.clear(); 
    for (framenum = 0; framenum < frameList->numActualFrames(); framenum++) {
      if (previousName != frameList->getFrame(framenum)->filename) {
	gRenderer->files.push_back(frameList->getFrame(framenum)->filename); 
	previousName = frameList->getFrame(framenum)->filename;        
      }
    }
	
    /* Tell back-end instances to load the files */
    for (i = 0; i < gRenderer->dmxScreenInfos.size(); i++) {
	  if (gRenderer->dmxWindowInfos[i].window) {
		gRenderer->mActiveSlaves[gRenderer->dmxWindowInfos[i].screen]->
		  SendFrameList(gRenderer->files);
	  }
    }
 }
 
void dmx_SetupPlay(int play, int preload, 
                   uint32_t startFrame, uint32_t endFrame) {
  ECHO_FUNCTION(5);
  uint32_t i = 0; 
  for (i = 0; i < gRenderer->dmxScreenInfos.size(); i++) {
    if (gRenderer->dmxWindowInfos[i].window) {
      gRenderer->mActiveSlaves
        [gRenderer->dmxWindowInfos[i].screen]->
        SendMessage(QString("SetPlayDirection %1 %2 %3 %4")
                    .arg(play).arg(preload).arg(startFrame).arg(endFrame));
    }
  }
  return; 
}
 
void   dmx_DestroyRenderer(Canvas *canvas){
  ECHO_FUNCTION(5);
   if (!gRenderer->numValidWindowInfos) return; 
   if (canvas != NULL) {
	 
     int i;
     for (i = 0; i < gRenderer->numValidWindowInfos; i++) {
       /* why send this message?  It does nothing in the slave! */ 
       gRenderer->mActiveSlaves[gRenderer->dmxWindowInfos[i].screen]->
         SendMessage( QString("Destroy Canvas"));
     }
     if (gRenderer->dmxWindowInfos)
       delete [] gRenderer->dmxWindowInfos;
   }
   
 }
 
 
/*
 * This function creates the back-end windows/canvases on the back-end hosts.
 *
 * The backend canvases should create child windows of the
 * window created by DMX (to work around NVIDIA memory issues,
 * and GLX visual compatibility).
 *
 * Also, update the subwindow sizes and positions as needed.
 * This is called when we create a canvas or move/resize it.
 */
void UpdateBackendCanvases(Canvas *canvas)
{
    
   if (!gRenderer->numValidWindowInfos) return; 

   DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;
    int i;

    for (i = 0; i < gRenderer->numValidWindowInfos; i++) {
	  const int scrn = gRenderer->dmxWindowInfos[i].screen;
	  
	  if (!gRenderer->haveDMX) {
		ERROR("UpdateBackendCanvases: no not have DMX!"); 
		abort(); 
	  }
	  /*
	   * Check if we need to create any back-end windows / canvases
	   */
	  if (gRenderer->dmxWindowInfos[i].window && !gRenderer->mActiveSlaves[scrn]->HaveCanvas()) {
		gRenderer->mActiveSlaves[scrn]->
		  SendMessage(QString("CreateCanvas %1 %2 %3 %4 %5 %6 %7")
					  .arg( gRenderer->dmxScreenInfos[scrn]->displayName)
					  .arg((int) gRenderer->dmxWindowInfos[i].window)
					  .arg(gRenderer->dmxWindowInfos[i].pos.width)
					  .arg(gRenderer->dmxWindowInfos[i].pos.height)
					  .arg(canvas->depth)
					  .arg(glueInfo->frameCacheSize)
					  .arg(glueInfo->readerThreads));

		gRenderer->mActiveSlaves[scrn]->HaveCanvas(true);
		if (gRenderer->files.size()) {
		  /* send list of image files too */
		  if (gRenderer->dmxWindowInfos[i].window) {
			gRenderer->mActiveSlaves[scrn]->SendFrameList(gRenderer->files);
		  }
		}
	  }
	  
	  /*
	   * Compute new position/size parameters for the backend subwindow.
	   * Send message to back-end processes to resize/move the canvases.
	   */
	  if (i < gRenderer->numValidWindowInfos && gRenderer->dmxWindowInfos[i].window) {
		gRenderer->mActiveSlaves[scrn]->
		  MoveResizeCanvas(gRenderer->dmxWindowInfos[i].vis.x,
						   gRenderer->dmxWindowInfos[i].vis.y,
						   gRenderer->dmxWindowInfos[i].vis.width,
						   gRenderer->dmxWindowInfos[i].vis.height);
	  }
    }
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
        GetBackendInfo(canvas);
    }

    UpdateBackendCanvases(canvas);
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
    GetBackendInfo(canvas);
  }
  
  UpdateBackendCanvases(canvas);
  
  /* I think these are redundant, as MoveXWindow sets them
     canvas->XPos = newX;
     canvas->YPos = newY;
  */ 
} 


/*
 * If we're about to draw <imageRegion> at destX, destY in mural coordinates,
 * clip the <imageRegion> according to <vis> (the visible region on a
 * particular screen.
 */
void ClipImageRegion(int destX, int destY, const Rectangle *imageRegion,
                     const XRectangle *vis, float zoom,
                     int *destXout, int *destYout, Rectangle *regionOut)
{
    int dx, dy;

    /* Compute bounds of the image in mural space */
    int x0 = destX;
    int y0 = destY;
    int x1 = destX + (int)(imageRegion->width * zoom); /* plus epsilon? */
    int y1 = destY + (int)(imageRegion->height * zoom);

    /* Bounds of the image that's visible */
    int ix0 = imageRegion->x;
    int iy0 = imageRegion->y;
    int ix1 = imageRegion->x + imageRegion->width;
    int iy1 = imageRegion->y + imageRegion->height;

    /* initial dest position for this tile */
    dx = destX;
    dy = destY;

    /* X clipping */
    if (x1 < vis->x) {
        /* image is completely left of this tile */
        ix0 = ix1 = 0;
    }
    else if (x0 > vis->x + vis->width) {
        /* image is completely to right of this tile */
        ix0 = ix1 = 0;
    }
    else if (x1 > vis->x + vis->width) {
        /* right edge of image is clipped */
        int d = x1 - (vis->x + vis->width);
        x1 = vis->x + vis->width;
        ix1 -= (int)(d / zoom);
        if (x0 < vis->x) {
            /* left edge of image is also clipped */
            int d = vis->x - x0;
            x0 = vis->x;
            ix0 += (int)(d / zoom);
            dx = 0;
        }
        else {
            dx -= vis->x; /* bias by visible tile origin */
        }
    }
    else if (x0 < vis->x) {
        /* only left edge of image is clipped */
        int d = vis->x - x0;
        x0 = vis->x;
        ix0 += (int)(d / zoom);
        dx = 0;
    }
    else {
        /* no X clipping */
        /* bias dest pos by visible tile origin */
        bb_assert(x0 >= vis->x);
        bb_assert(x0 < vis->x + vis->width);
        dx -= vis->x;
    }

    /* Y clipping */
    if (y1 < vis->y) {
        /* image is completely above this tile */
        iy0 = iy1 = 0;
    }
    else if (y0 > vis->y + vis->height) {
        /* image is completely below this tile */
        iy0 = iy1 = 0;
    }
    else if (y1 > vis->y + vis->height) {
        /* bottom edge of image is clipped */
        int d = y1 - (vis->y + vis->height);
        y1 = vis->y + vis->height;
        iy1 -= (int)(d / zoom);
        if (y0 < vis->y) {
            /* top edge of image is also clipped */
            int d = vis->y - y0;
            y0 = vis->y;
            iy0 += (int)(d / zoom);
            dy = 0;
        }
        else {
            dy -= vis->y; /* bias by visible tile origin */
        }
    }
    else if (y0 < vis->y) {
        /* only top edge of image is clipped */
        int d = vis->y - y0;
        y0 = vis->y;
        iy0 += (int)(d / zoom);
        dy = 0;
    }
    else {
        /* no Y clipping */
        /* bias dest pos by visible tile origin */
        bb_assert(y0 >= vis->y);
        bb_assert(y0 < vis->y + vis->height);
        dy -= vis->y;
    }

    /* OK, finish up with new sub-image rectangle */
    regionOut->x = ix0;
    regionOut->y = iy0;
    regionOut->width = ix1 - ix0;
    regionOut->height = iy1 - iy0;
    *destXout = dx;
    *destYout = dy;

    /* make sure our values are good */
    bb_assert(regionOut->x >= 0);
    bb_assert(regionOut->y >= 0);
    bb_assert(regionOut->width >= 0);
    bb_assert(regionOut->height >= 0);
    bb_assert(dx >= 0);
    bb_assert(dy >= 0);
#if 0
    printf("  clipped window region: %d, %d .. %d, %d\n", x0, y0, x1, y1);
    printf("  clipped image region: %d, %d .. %d, %d\n", ix0, iy0, ix1, iy1);
    printf("  clipped render: %d, %d  %d x %d  at %d, %d  zoom=%f\n",
           x, y, w, h, dx, dy, zoom);
#endif
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
			
            ClipImageRegion(destX, destY, imageRegion, vis, zoom,
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

void  dmx_DrawString(Canvas *canvas, int row, int column, const char *str)
{
   ECHO_FUNCTION(5);
 if (!gRenderer->numValidWindowInfos) return; 
    DMXRendererGlue *glueInfo = (DMXRendererGlue *) canvas->gluePrivateData;
    
    int x = (column + 1) * glueInfo->fontHeight;
    int y = (row + 1) * glueInfo->fontHeight;
    int i;

    /* Send DrawString to back-end renderers, with appropriate offsets */
    for (i = 0; i < gRenderer->numValidWindowInfos; i++) {
        if (gRenderer->dmxWindowInfos[i].window) {
            const int scrn = gRenderer->dmxWindowInfos[i].screen;

            int tx = x - gRenderer->dmxWindowInfos[i].vis.x;
            int ty = y - gRenderer->dmxWindowInfos[i].vis.y;
            int tcol = tx / glueInfo->fontHeight - 1;
            int trow = ty / glueInfo->fontHeight - 1;

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
