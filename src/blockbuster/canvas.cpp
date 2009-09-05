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

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/times.h>
#include "canvas.h"
#include "ui.h"
#include "errmsg.h"
#include "cache.h"
#include "blockbuster_qt.h"
#include "xwindow.h"

/* This file handles the creation and destruction of the dynamic Canvas
 * objects, and encapsulates references to the Renderers, UserInterfaces,
 * and appropriate Glue.
 */



/* This is the master function to create a Canvas.  It plugs in the desired
 * UserInterface and Renderer and the appropriate Glue routines.
 */

Canvas::Canvas(qint32 parentWindowID, BlockbusterInterface *gui):
  height(0), width(0), screenHeight(0), screenWidth(0), 
  XPos(0), YPos(0), depth(0), threads(0), cachesize(0), 
  mBlockbusterInterface(gui), 
  Render(NULL), ImageDataAllocator(NULL), ImageDataDeallocator(NULL), 
  frameList(NULL), imageCache(NULL), SetFrameList(NULL), Preload(NULL), 
  DestroyRenderer(NULL), rendererPrivateData(NULL), Resize(NULL), Move(NULL), 
  DrawString(NULL), SwapBuffers(NULL), BeforeRender(NULL), AfterRender(NULL)
{
    RendererGlue *glue;
    MovieStatus status;
    ProgramOptions *options = GetGlobalOptions(); 
    UserInterface *userInterface = options->userInterface; 
    glue = userInterface->supportedRenderers[options->rendererIndex];

    /* We've got a UserInterface, a Renderer, and glue.  We're good to go. 
     * The UserInterface gets to go first, because it has to open the window
     * or widget (and may adjust the desired size appropriately), and it has
     * to prepare the Glue required for the Renderer to function.
     */

	this->threads = options->readerThreads;
	this->cachesize = options->frameCacheSize;

    /* Initialize the UserInterface.  The "wart" uiData parameter is used
     * in the master/remote slave case to pass a parent window ID to the
     * slave UI.  Once the "slave" UI is created, the uiData field will
     * be removed, leaving a UI-specific option.
     */
    
    status = userInterface->Initialize(this, options, parentWindowID, glue->configurationData);
    if (status != MovieSuccess) {
      cerr << "Cannot initialize userInterface struct in canvas constructor" << endl; 
      exit (1); 
    }
    
    /* Renderer can initialize now. */
    status = glue->renderer->Initialize(this, options);

    /* If this renderer would like an image cache, we can create one, as
     * a convenience (this is done because most renderers do use an 
     * image cache; putting the cache creation here simplifies their
     * implementations).  If the UserInterface or Glue routines supply
     * their own methods, we refuse to override them.
     */
    DEBUGMSG(QString("frameCacheSize is %1, preload is %2, setFrameList is %3").arg(options->frameCacheSize).arg((int64_t)Preload).arg((int64_t)SetFrameList)); 
    if (options->frameCacheSize > 0 && this->Preload == NULL && this->SetFrameList == NULL) {
      this->imageCache = CreateImageCache(options->readerThreads,
                                          options->frameCacheSize, this);
      if (this->imageCache == NULL) {
	    WARNING("could not allocate image cache");
	    this->Preload = NULL;
	    this->SetFrameList = DefaultSetFrameList;
      }
      else {
	    this->Preload = CachePreload;
	    this->SetFrameList = CacheSetFrameList;
      }
    }
    
    
    /* All done */
    return ;
}

Canvas::~Canvas()
{
    if (this->DestroyRenderer != NULL) {
      this->DestroyRenderer(this);
    }
    if (this->imageCache != NULL) {
      DestroyImageCache(this);
    }
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
    XWindow_SetTitle(QString("Blockbuster (%1)").arg(moviename)); 
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
FrameInfo *GetFrameInfoPtr(Canvas *canvas, int frameNumber)
{
  /* Added to support stereo files */
  /* Assumes canvas has a valid FrameList */
  int localFrameNumber = 0;

  if(canvas->frameList->stereo) {
	localFrameNumber = frameNumber * 2;
	
  }
  else {
	localFrameNumber = frameNumber;
  }
  
  return (FrameInfo*)canvas->frameList->getFrame(localFrameNumber);
}

/* These routines are typically used by any Canvas that uses the
 * ImageCache utilities.
 */
void CacheSetFrameList(Canvas *canvas, FrameList *frameList)
{
#if 1
    /* Optional: completely destroy the image cache, and 
     * recreate it.  This will destroy threads and whatnot,
     * and may prevent a problem with switching movies
     * quickly.  (We really should instead discover what
     * state is causing the breakdown, but this will
     * allow testing for now.)
     */
   
	
#if 0
	/* we disabled the following since we need to destroy cache in an earlier phase */
	 /* Save the old ImageCache state that we'll need for the
     * new ImageCache.  
     */
    /*tCacheConfiguration(canvas->imageCache, &numReaderThreads, &maxCachedImages); */

    /* Destroy the cache. */
    /* troyImageCache(canvas->imageCache); */

#endif

    /* Recreate it. */
	/*fprintf(stderr,"Recreate Image Cache -- Size %d Threads %d\n",canvas->cachesize,canvas->threads); */
    if (!canvas->imageCache) {
      canvas->imageCache = CreateImageCache(canvas->threads, canvas->cachesize, canvas);
    }
    if (canvas->imageCache == NULL) {
	ERROR("could not recreate image cache when changing frame list");
	canvas->Preload = NULL;
	canvas->SetFrameList = DefaultSetFrameList;

	/* Invoke the new framelist function, and get out of here. */
	canvas->SetFrameList(canvas, frameList);
	return;
    }
#endif

    /* Tell the cache to manage the new frame list.  This will
     * clear everything out of the cache that's already in there.
     */
    canvas->imageCache->ManageFrameList(frameList);
    canvas->frameList = frameList;
}

/* And this one is typically used by a Canvas that doesn't use
 * the ImageCache utilities.
 */
void DefaultSetFrameList(Canvas *canvas, FrameList *frameList)
{
    canvas->frameList = frameList;
}
