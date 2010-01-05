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

#include "blockbuster_qt.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/times.h>
#include "canvas.h"
#include "errmsg.h"
#include "cache.h"
#include "xwindow.h"
#include "dmxRenderer.h"
#include "Renderer.h"


Canvas::Canvas(qint32 parentWindowID, ProgramOptions *options, 
               BlockbusterInterface *gui):
  height(0), width(0), screenHeight(0), screenWidth(0), 
  XPos(0), YPos(0), depth(0), threads(0), cachesize(0), 
  mBlockbusterInterface(gui), 
  frameList(NULL), 
  ResizePtr(NULL), MovePtr(NULL), 
  DrawStringPtr(NULL), SwapBuffersPtr(NULL), 
  BeforeRenderPtr(NULL),  mOptions(options)
{

	this->threads = mOptions->readerThreads;
	this->cachesize = mOptions->frameCacheSize;

    
    /* Renderer can initialize now. */
    mRenderer = NewRenderer::CreateRenderer(mOptions, this, parentWindowID); 
    if (!mRenderer) {
      ERROR(QString("Badness:  cannot create renderer \"%1\"\n").
            arg(mOptions->rendererName)); 
      exit(1); 
    }
    mOptions->mNewRenderer = mRenderer; 

    DEBUGMSG(QString("frameCacheSize is %1").arg(mOptions->frameCacheSize));    
    
    /* All done */
    return ;
}

Canvas::~Canvas()
{
  if (mRenderer) delete mRenderer; 
  
  return; 
}


void Canvas::ReportFrameChange(int frameNumber) {
  DEBUGMSG("Canvas::ReportFrameChange %d", frameNumber); 
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameNumber(frameNumber+1); 
  }
  return; 
}

void Canvas::ReportFrameListChange(const FrameList *frameList) {
  DEBUGMSG("Canvas::ReportFrameListChange"); 
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameRange(1, frameList->numStereoFrames()); 
    mBlockbusterInterface->setFrameNumber(1); 
    QString moviename = frameList->getFrame(0)->filename; 
    if (moviename.size() > 32) moviename = QString("...") + moviename.right(32); 
    mBlockbusterInterface->setTitle(QString("Blockbuster Control (%1)").arg(moviename)); 
    mRenderer->SetTitle(QString("Blockbuster (%1)").arg(moviename)); 
  }
  return; 
}

void Canvas::ReportDetailRangeChange(int min, int max) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setLODRange(min,max); 
    mBlockbusterInterface->setLOD(1); 
  }
  return; 
}

void Canvas::ReportDetailChange(int levelOfDetail) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setLOD(levelOfDetail); 
  }
  return; 
}

void Canvas::ReportRateRangeChange(float min, float max) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameRateRange(min,max); 
  }
  return; 
}

void Canvas::ReportLoopBehaviorChange(int behavior) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setLoopBehavior(behavior); 
  }
  return; 
}

void Canvas::ReportPingPongBehaviorChange(int behavior) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setPingPongBehavior(behavior); 
  }
  return; 
}

void Canvas::ReportRateChange(float rate) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameRate(rate); 
  }
  return; 
}

void Canvas::ReportZoomChange(float zoom) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setZoom(zoom); 
  }
  return; 
}

void Canvas::ShowInterface(int on) {
  if (mBlockbusterInterface) {
    if (on) {
      mBlockbusterInterface->show(); 
    } else {
      mBlockbusterInterface->hide(); 
    }
  }
  return; 
}
//============================================
void Canvas::reportWindowMoved(int xpos, int ypos){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportWindowMoved(xpos, ypos); 
  }
  return ;
}

//============================================
void Canvas::reportWindowResize(int x, int y){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportWindowResize(x,y); 
  }
  return ;
} 

//============================================
void Canvas::reportMovieMoved(int xpos, int ypos){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieMoved(xpos, ypos); 
  }
  return ;
} 
 
//============================================
void Canvas::reportMovieFrameSize(int x, int y){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieFrameSize(x,y);
  }
  return ;
} 

//============================================
void Canvas::reportMovieDisplayedSize(int x, int y){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieDisplayedSize(x,y); 
  }
  return ;
} 

//============================================
void Canvas::reportActualFPS(double rate){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportActualFPS(rate); 
  }
  return ;
}

//============================================
void Canvas::reportMovieCueStart(void){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieCueStart(); 
  }
  return ;
}

//============================================
void Canvas::reportMovieCueComplete(void){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieCueComplete(); 
  }
  return ;
}
//============================================
void Canvas::DMXSendHeartbeat(void) { 
  // this is probably better done with QTimer. 
  if (mRenderer->mName == "dmx") {
    dynamic_cast<dmxRenderer *>(mRenderer)->SendHeartbeatToSlaves(); 
  }
  return; 
}

//============================================
void Canvas::DMXSpeedTest(void) {
  if (mRenderer->mName == "dmx") {
    dynamic_cast<dmxRenderer*>(mRenderer)->SpeedTest(); 
  }
  return; 
}

//============================================
void Canvas::DMXCheckNetwork(void) {
  if (mRenderer->mName == "dmx") {
    dynamic_cast<dmxRenderer*>(mRenderer)->CheckNetwork(); 
  }
  return; 
}

//============================================
FrameInfo *Canvas::GetFrameInfoPtr(int frameNumber)
{
  /* Added to support stereo files */
  /* Assumes canvas has a valid FrameList */
  int localFrameNumber = 0;

  if(frameList->stereo) {
	localFrameNumber = frameNumber * 2;
	
  }
  else {
	localFrameNumber = frameNumber;
  }
  
  return (FrameInfo*)frameList->getFrame(localFrameNumber);
}
