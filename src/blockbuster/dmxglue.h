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

#ifndef _DMXGLUE_H
#define _DMXGLUE_H 1

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMutex>
#include <QProcess>
#include <QHostInfo>
#include <deque>
#include <vector>
#include <iostream>
#include "events.h"
#include "canvas.h"
#include "frames.h"
#include "errmsg.h"
//#include "Renderers.h"
using namespace std; 

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "canvas.h"


#define DMX_NAME "dmx"
#define DMX_DESCRIPTION "Directly render to back-end DMX servers: Use -R to specify\n         backend renderer"
MovieStatus dmx_Initialize(Canvas *canvas, const ProgramOptions *options);
void dmx_Resize(Canvas *canvas, int newWidth, int newHeight, int cameFromX);
void dmx_Render(Canvas *, int frameNumber,
           const Rectangle *imageRegion,
           int destX, int destY, float zoom, int lod);
void dmx_Move(Canvas *canvas, int newX, int newY, int cameFromX);
void dmx_DestroyRenderer(Canvas *canvas); 
void dmx_SwapBuffers(Canvas *canvas);
void dmx_DrawString(Canvas *canvas, int row, int column, const char *str); 
int IsDMXDisplay(Display *dpy);
void GetBackendInfo(Canvas *canvas);
void UpdateBackendCanvases(Canvas *canvas);

/* This file details the structure that the DMX Renderer requires
 * in order to render.  It is used by the DMX Renderer itself
 * (naturally), and also any UserInterface that wishes to support
 * the X11 Renderer (through the "glue" routines associated wtih
 * the UserInterface and the Renderer).  It contains all the information
 * from the UserInterface that the Renderer needs to render.
 *
 * A structure of this sort must be loaded into the Canvas'
 * gluePrivateData pointer during "glue" initialization.
 */

 struct DMXRendererGlue {
    Display *display;
    Window window;
    int fontHeight;
    int frameCacheSize;
    int readerThreads;
} ;

/*!
  This shall be set to be called at exit.  It looks at the last known renderInfo and deletes all the slaves that might be running. 
*/ 
 void dmx_AtExitCleanup(void);


/* kind of bogus, but gets me by for now */  
void dmx_SetFrameList(Canvas *canvas, FrameList *frameList);


/* thump-thump */
void dmx_SendHeartbeatToSlaves(void); 

/* Test out a new way to do DMX:
   this makes the slaves just start playing and ignore Render commands 
*/ 
void dmx_SpeedTest(void); 



#endif
