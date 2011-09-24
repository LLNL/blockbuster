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
#include "qmetatype.h"
#include "QInputDialog"
#include "QDir"
#include "blockbuster_qt.h"
#include "timer.h"
#include "cache.h"
#include "splash.h"
#include "SidecarServer.h"
#include "util.h"
#include "remotecommands.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/times.h>
#include <sys/types.h>
#include "movie.h"
#include "errmsg.h"
#include "frames.h"
#include "../libpng/pngsimple.h"
#include <libgen.h>
#include "xwindow.h"
#include "canvas.h"
/* ---------------------------------------------*/ 

static void FrameListModified(Canvas *canvas, FrameList *frameList)
{
  canvas->SetFrameList(frameList);
  canvas->ReportFrameListChange(frameList);

  
}


/*
 * Given a zoom factor, compute appropriate level of detail.
 */
int LODFromZoom(float zoom)
{
    /* XXX this may need tuning */
    int lod = 0;
    while (zoom <= 0.5) {
       zoom *= 2.0;
       lod++;
    }
    return lod;
}

static float ComputeZoomToFit(Canvas *canvas, int width, int height)
{
  float xZoom, yZoom, zoom;

  xZoom = (float) canvas->width / width;
  yZoom = (float) canvas->height / height;
  zoom = xZoom < yZoom ? xZoom : yZoom;

  /* The return value is the "integer zoom", a value which
   * matches well how the user interface is handling zooming.
   * It will be 0 for no zooming, >0 if the canvas is bigger
   * than the image and so the image has to be expanded, or
   * <0 if the canvas is smaller than the image and the
   * image has to be shrunk.
   */
  /*RDC -- what is with all these 500's ?  */ 
  // return (int) ((zoom - 1.0) * 500.0);
  return zoom; 
}


void ClampStartEndFrames(FrameList *allFrames, 
						 int32_t &startFrame, 
						 int32_t &endFrame, 
						 int32_t &frameNumber, 
						 bool warn = false) {
  DEBUGMSG(QString("BEGIN ClampStartEndFrames(%1, %2, %3, %4...").arg(allFrames->getFrame(0)->filename).arg(startFrame).arg(endFrame).arg(frameNumber)); 
  if (endFrame <= 0) {
    endFrame = allFrames->numStereoFrames()-1;
  }
  if (startFrame < 0) {
    startFrame = 0;
  }
  if (startFrame > static_cast<int32_t>(allFrames->numStereoFrames()) -1){
    if (warn) WARNING("startFrame value of %d is greater than number of frames in movie (%d), clamping to last frame", startFrame, allFrames->numStereoFrames()-1); 
    startFrame = allFrames->numStereoFrames() -1;
  }
  if (endFrame > static_cast<int32_t>(allFrames->numStereoFrames()) -1){
    if (warn) WARNING("endFrame value of %d is greater than number of frames in movie (%d), clamping to last frame", endFrame, allFrames->numStereoFrames()-1); 
    endFrame = allFrames->numStereoFrames() -1;
  }
  if (endFrame < startFrame) {
    // always warn about this: 
    WARNING("endFrame value of %d is less than endFrame value (%d), reversing then", endFrame, startFrame);
    int32_t tmp = endFrame; 
    endFrame = startFrame; 
    startFrame = tmp; 
  }
  if (frameNumber > endFrame) frameNumber = endFrame; 
  if (frameNumber < startFrame) frameNumber = startFrame; 
  DEBUGMSG(QString("END ClampStartEndFrames(%1, %2, %3, %4...").arg(allFrames->getFrame(0)->filename).arg(startFrame).arg(endFrame).arg(frameNumber)); 
  return;
}

/*
 * Main UI / image display loop
 * XXX this should get moved to a new file.
 */
int DisplayLoop(FrameList *allFrames, ProgramOptions *options)
{
  int32_t previousFrame = -1, cueEndFrame = 0;
  uint totalFrameCount = 0, recentFrameCount = 0;
  FrameInfo *frameInfo = NULL;
  Canvas *canvas;
  int maxWidth, maxHeight, maxDepth;
  int loopCount = options->loopCount; 
  int drawInterface = options->drawInterface;
  int skippedDelayCount = 0, usedDelayCount = 0;
  int done =0;
  /* Timing information */
  struct tms startTime, endTime;
  clock_t startClicks, endClicks;
  long Hertz;
  double recentStartTime = GetCurrentTime(), 
    recentEndTime, elapsedTime, userTime, systemTime;
  double fps = 0.0;
  double nextSwapTime = 0.0;
  float targetFPS = 0.0;

  double noscreensaverStartTime = GetCurrentTime();

  bool pingpong = false; // play forward, then backward, then forward, etc
  bool cuePlaying = false;  // when true, status will not change. 
  /* UI / events */
  int xOffset = 0, yOffset = 0, oldXOffset = 0, oldYOffset = 0; // image position relative to center of canvas
  int32_t panning = 0, panStartX = 0, panStartY = 0, panDeltaX = 0, panDeltaY = 0;
  int zooming = 0, zoomStartY = 0, zoomDelta = 0;  // integer zoom for mouse
  /*int izoom = 0*/  // for mouse zoom
  float currentZoom = 1.0, newZoom, startZoom, oldZoom; // actual zoom factor
  int lod = 0, maxLOD, baseLOD = 0, lodBias = options->LOD;
  bool usingDmx = (QString(options->rendererName) == "dmx"); 
  long sleepAmt=1; // for incremental backoff during idle
  /* Region Of Interest of the image */
  Rectangle roi;
  int destX, destY; // position on canvas to render the image

  
 /* We'll need this for timing */
  Hertz = sysconf(_SC_CLK_TCK);

  /* Insert splash screen code here. 
   */


  allFrames->GetInfo(maxWidth, maxHeight, maxDepth, maxLOD, targetFPS);
  if (options->frameRate != 0.0) targetFPS = options->frameRate; 
  options->frameRate = targetFPS; 

  if(options->decorations) {
    if (options->geometry.width == DONT_CARE)
      options->geometry.width = maxWidth;
    if (options->geometry.height == DONT_CARE)
      options->geometry.height = maxHeight;
  }

  canvas = new Canvas(0, options, gMainWindow);
  gSidecarServer->SetCanvas(canvas); 

  if (canvas == NULL) {
    ERROR("Could not create a canvas");
    return 1;
  }
  int32_t preloadFrames= options->preloadFrames,
    playDirection = 0, 
    startFrame= options->startFrame, 
    frameNumber = options->currentFrame, 
    endFrame = options->endFrame; 
  
  /* Tell the Canvas about the frames it's going to render.
    Generate a warning if the frames are out of whack */
  FrameListModified(canvas, allFrames);
  ClampStartEndFrames(allFrames, startFrame, endFrame, frameNumber, true); 
  int playExit = options->playExit; 
  if (playExit < 0) playExit = endFrame; 
  
  /* The Canvas may or may not have an interface to display
   * (or to hide, if we chose to initially have the interface
   * off)
   */
  //  canvas->ReportRateRangeChange(0.01, 1000);
  canvas->ReportFrameChange(frameNumber);
  canvas->ReportDetailRangeChange(-maxLOD, maxLOD);
  canvas->ShowInterface(options->drawInterface);
  canvas->ReportDetailChange(lodBias);
  canvas->ReportLoopBehaviorChange(loopCount); 

  /* Compute a starting zoom factor.  We won't do a full 
   * Zoom to Fit, but we will do a Shrink to Fit (i.e.
   * we'll allow the movie to shrink, but not to grow), so that
   * the new movie fits in the Canvas initially.
   */
  newZoom = ComputeZoomToFit(canvas, maxWidth, maxHeight);
  if (newZoom > 1.0) {
    /* The canvas is larger - do a normal zoom at startup */
    newZoom = 1.0;
  }


  /* Start timing */
  startClicks = times(&startTime);
  recentStartTime = GetCurrentTime();

  if ( options->play != 0 )
    playDirection = options->play;


  if ( options->zoom != 1.0 )
    newZoom =options->zoom;

  canvas->ReportRateChange(options->frameRate);
					
  time_t lastheartbeat = time(NULL); 
  bool fullScreen = options->fullScreen, zoomOne = false; 
  MovieSnapshot oldSnapshot; 


  while(! done) {
    // TIMER_PRINT("loop start"); 
    vector<MovieEvent> events;
    MovieEvent newEvent; 
    int oldPlay = playDirection;

    if (fullScreen && options->fullScreen) {
      DEBUGMSG("fullScreen from options\n"); 
      events.push_back(MovieEvent(MOVIE_FULLSCREEN)); 
      options->fullScreen=false; 
    }
    /* Get an event from the canvas or network and process it.
     */   
    /* every minute or so, tell any DMX slaves we are alive */ 
    if (time(NULL) - lastheartbeat > 60) {
      canvas->DMXSendHeartbeat(); 
      lastheartbeat = time(NULL); 
    }
 
    gCoreApp->processEvents(); 
    if (gMainWindow->GetEvent(newEvent)) {  
      events.push_back(newEvent); 
    }
    if (GetNetworkEvent(&newEvent)) { /* Qt events from e.g. Sidecar */
      events.push_back(newEvent); 
    } 
    canvas->mRenderer->GetXEvent(0, &newEvent); 
    if (playDirection && newEvent.eventType == MOVIE_GOTO_FRAME && 
        newEvent.number == frameNumber+playDirection) {
      newEvent.eventType = MOVIE_NONE; 
    }   
    events.push_back(newEvent); 
    
    //canvas->GetEvent(canvas, 0, &newEvent);
 
   // we now have at least one event
    uint32_t eventNum = 0; 
    while (eventNum < events.size()) {
      bool swapBuffers = false; 
     MovieEvent event = events[eventNum];
     bool sendSnapshot = false; 
     if (event.eventType != MOVIE_NONE) {
       DEBUGMSG("Got %s\n", event.Stringify().c_str()); 
     }
      eventNum ++; 
      switch(event.eventType) {
      case   MOVIE_SIDECAR_STATUS:   
      case   MOVIE_SNAPSHOT:   
      case   MOVIE_SNAPSHOT_STARTFRAME:   
      case   MOVIE_SNAPSHOT_ENDFRAME:   
      case   MOVIE_SNAPSHOT_ALT_ENDFRAME:   
        sendSnapshot = true; 
        break; 
      case   MOVIE_SIDECAR_BACKCHANNEL:       
        gSidecarServer->connectToSidecar(QString(event.mString)); 
        break; 
      case   MOVIE_CUE_PLAY_ON_LOAD: 
        playDirection = event.height; 
        break;
      case   MOVIE_CUE_BEGIN: 
        cueEndFrame = event.number;  // for now, a cue will be defined to be "executing" until the end frame is reached, then the cue is complete
        canvas->reportMovieCueStart(); 
        break; 
      case   MOVIE_CUE_END: 
        /* This event is just the end of the cue data stream, and is
           not related to when the cue is done executing.  
           In fact, the cue is not really playing until now.  
        */ 
        cuePlaying = true;
        break; 
      case   MOVIE_CUE_MOVIE_NAME: 
      case   MOVIE_CUE_PLAY_BACKWARD: 
        break; 
      case MOVIE_PWD:
        {
          QString pwd =  ParentDir(QString(frameInfo->filename));
          gSidecarServer->SendEvent(MovieEvent(MOVIE_PWD, pwd)); 
        }
        break; 
      case MOVIE_START_END_FRAMES:
        /*  Store this new event as the user's new global preference */
        options->startFrame = event.width;
        options->endFrame = event.height; 
        // apply the start and end frames to the current movie
        startFrame = options->startFrame;
        endFrame = options->endFrame;
        // clamp frame values, generate a warning if they are funky
        ClampStartEndFrames(allFrames, startFrame, endFrame, frameNumber, true); 
        canvas->ReportFrameChange(frameNumber); 
        DEBUGMSG("START_END_FRAMES: start %d end %d current %d\n", startFrame, endFrame, frameNumber); 
        break; 
      case MOVIE_MESSAGE:
        DEBUGMSG("MOVIE_MESSAGE event: %s\n", event.mString); 
        break; 
      case MOVIE_QUIT:
		done = 1;
        break;
      case MOVIE_IMAGE_MOVE:
        xOffset = -event.x; 
        yOffset = event.y; 
        break; 
      case MOVIE_FULLSCREEN:
        //DEBUGMSG("fullscreen"); canvas->width
        if(frameInfo != NULL) {
          /*newZoom = ComputeZoomToFit(canvas, frameInfo->width,
                                   frameInfo->height);
          */
          newZoom = ComputeZoomToFit(canvas, canvas->width,
                                   canvas->height);
          //DEBUGMSG("Zoom to Fit: %f", newZoom);
        } else {
          //DEBUGMSG("No zooming done"); 
        }
        fullScreen = true; 
        // fall through to MOVIE_MOVE_RESIZE: 
        event.eventType = MOVIE_MOVE_RESIZE;
        event.width = canvas->screenWidth; 
        event.height = canvas->screenHeight; 
        event.x = 0; 
        event.y = 0; 
        goto MOVIE_MOVE_RESIZE; 
      case MOVIE_MOVE:
      case MOVIE_RESIZE:
      case MOVIE_MOVE_RESIZE:
      MOVIE_MOVE_RESIZE:
        if (0) // (event.eventType == MOVIE_RESIZE || event.eventType == MOVIE_MOVE_RESIZE) 
          {
          zoomOne = fullScreen = false; 
        }
        // The code below moves, resizes or does both as appropriate.  Doing it this way was easier than making two new functions, so sue me.  
        // =====================================
        // MOVE ===========================
        if (event.eventType == MOVIE_MOVE || event.eventType == MOVIE_MOVE_RESIZE) {
          //DEBUGMSG("MOVE"); 
          if (event.x == -1) 
            event.x = canvas->XPos;
          if (event.y == -1) 
            event.y = canvas->YPos;
          canvas->Move(event.x, event.y, event.number);            
        }
        // END MOVE ===========================
        
        // ====================================
        
        // RESIZE ===========================
        if (event.eventType == MOVIE_RESIZE || event.eventType == MOVIE_MOVE_RESIZE) {
            //DEBUGMSG("RESIZE"); 
          if (event.width == 0) {
            /* if there are frames, use the frame width for the window width 
               else use the current width */ 
            if(frameInfo != NULL) {
              event.width = frameInfo->width;
            } else {
              event.width = canvas->width;
            }
          }         
          /* if (event.height == -1 )  {
          // use the whole screen height  
          event.height = canvas->screenHeight; 
          } else */
          if (event.height == 0)  {
            /* if there are frames, use the frame height for the window height 
               else use the current height */ 
            if(frameInfo != NULL) {
              event.height = frameInfo->height;
            } else {
              event.height = canvas->height;
            }
          }
          if (event.width > canvas->screenWidth) {
            event.width = canvas->screenWidth; 
          }
          if (event.height > canvas->screenHeight) {
            event.height = canvas->screenHeight; 
          }
          canvas->Resize(event.width,
                         event.height, event.number);
          /* Set these in case there is no Resize function */
          canvas->width = event.width;
          canvas->height = event.height;
          // If we have zoomFit set, resize the frame to fit in the newly resized
          // window.
          if (options->zoomFit/* && !event.number*/) {
            goto MOVIE_ZOOM_FIT;
          }
        }
        // END RESIZE ===========================
        swapBuffers = true; 
        break; // END MOVIE_MOVE, MOVIE_RESIZE OR MOVIE_MOVE_RESIZE
        
      case MOVIE_EXPOSE:
        /* Fall-through to rendering code */
        break;

      case MOVIE_NONE:
        break; 

      case MOVIE_SET_PINGPONG:
        pingpong = event.number;
        canvas->ReportPingPongBehaviorChange(event.number); 
        break; 
      case MOVIE_SET_LOOP:
        loopCount = event.number;    
        if (loopCount == 1) loopCount = 2; // this loops once. 
        else if (loopCount == 0) loopCount = 1; // this plays once, no loop
        options->loopCount = loopCount; 
        if (loopCount) {
          pingpong = false;
        }
        canvas->ReportLoopBehaviorChange(loopCount); 
        break;
      case MOVIE_PLAY_FORWARD: 
        playDirection = 1; loopCount = options->loopCount; 
        break;
      case MOVIE_PLAY_BACKWARD: 
        playDirection = -1; loopCount = options->loopCount; 
        break;
      case MOVIE_STOP: 
        DEBUGMSG("Got stop event"); 
        playDirection = 0; 
        break;
      case MOVIE_STOP_ERROR: 
        DEBUGMSG("Got stop error"); 
        playDirection = 0; 
        gSidecarServer->SendEvent(MovieEvent(MOVIE_STOP_ERROR, event.mString));
        continue;
      case MOVIE_PAUSE: 
        playDirection = !playDirection;
        break;
      case MOVIE_STEP_FORWARD: frameNumber++; break;
      case MOVIE_STEP_BACKWARD: frameNumber--; break;
      case MOVIE_SKIP_FORWARD: frameNumber += 20; break;
      case MOVIE_SKIP_BACKWARD: frameNumber -= 20; break;
      case MOVIE_SECTION_FORWARD: 
        frameNumber += MAX2(allFrames->numStereoFrames() / 4, 1);
        break;
      case MOVIE_SECTION_BACKWARD:
        frameNumber -= MAX2(allFrames->numStereoFrames() / 4, 1);
        break;
        
      case MOVIE_GOTO_START:
        frameNumber = startFrame;
        break;
        
      case MOVIE_GOTO_END:
        frameNumber = endFrame;
        break;
      case MOVIE_GOTO_FRAME:
        frameNumber = event.number;
        if (frameNumber < 0) {
          /* special meaning to SideCar: go to the last frame */
          frameNumber = endFrame;
        } 
        if (frameNumber < startFrame) {
          frameNumber = startFrame; 
        }
        break;
        
      case MOVIE_ZOOM_FIT: 
      MOVIE_ZOOM_FIT: 
        if(frameInfo != NULL) {
          newZoom = ComputeZoomToFit(canvas, frameInfo->width,
                                   frameInfo->height);
          DEBUGMSG("Zoom to Fit: %f", newZoom);
        } else {
          // Caution:  RDC: Zooming was not working right upon movie startup, so here I'm reusing some old code that was commented out -- maybe assumption that allFrames->frames[0] is not NULL is not valid?  
          //bb_assert (allFrames->frames[0] != NULL); 
          newZoom = ComputeZoomToFit(canvas,
                                     allFrames->getFrame(0)->width,
                                     allFrames->getFrame(0)->height);
        }
        xOffset = yOffset = 0;
        options->zoomFit = true; 
        break;
      case MOVIE_ZOOM_ONE:
        options->zoomFit = false; 
        zoomOne = true; 
        newZoom = 1.0;
        zooming = 0;       
        break;
      case MOVIE_ZOOM_SET:
        options->zoomFit = false; 
        newZoom = event.rate;
        if (newZoom == 0.0) newZoom = 1.0; 
        //zooming = 0;
        break;
      case MOVIE_ZOOM_UP:
        options->zoomFit = false; 
        newZoom = 1.2*currentZoom;
        zooming = 0;
        break;
      case MOVIE_ZOOM_DOWN:
        options->zoomFit = false; 
        newZoom = 0.8*currentZoom;
        if (newZoom < 0.05) newZoom = 0.05; 
        zooming = 0;
        break;
        
      case MOVIE_CENTER:
        xOffset = yOffset = 0;
        panning = 0;
        break;
      case MOVIE_TOGGLE_CURSOR:
        canvas->mRenderer->ToggleCursor(); 
        break; 
      case MOVIE_NOSCREENSAVER:
        options->noscreensaver = event.number;
        noscreensaverStartTime = GetCurrentTime();
       break; 
      case MOVIE_MOUSE_PRESS_1:
        panning = 1;
        panStartX = event.x;
        panStartY = event.y;
        panDeltaX = 0;
        panDeltaY = 0;
        break;
      case MOVIE_MOUSE_RELEASE_1:
        if (!panning) break; // avoid spurious signals
        panDeltaX = static_cast<int>((panStartX - event.x) / currentZoom);
        panDeltaY = static_cast<int>((panStartY - event.y) / currentZoom);
        xOffset += panDeltaX;
        yOffset += panDeltaY;
        panDeltaX = 0;
        panDeltaY = 0;
        panning = 0;
        break;
      case MOVIE_MOUSE_PRESS_2:
        zoomStartY = event.y;
        startZoom = currentZoom; 
        zooming = 1;
        break;
      case MOVIE_MOUSE_RELEASE_2:
        //newZoom += zoomDelta;
        newZoom = startZoom*(1+(float)zoomDelta/(canvas->height));
        zoomDelta = 0;
        zooming = 0;
        break;
      case MOVIE_MOUSE_MOVE:
        if (panning) {
          panDeltaX = static_cast<int>((panStartX - event.x) / currentZoom);
          panDeltaY = static_cast<int>((panStartY - event.y) / currentZoom);
        }
        else if (zooming) {          
          zoomDelta = zoomStartY - event.y;
          newZoom = startZoom*(1+(float)zoomDelta/(canvas->height));
        }
        break;
      case MOVIE_INCREASE_RATE:
        targetFPS += 1.0;
        canvas->ReportRateChange(targetFPS);
        
        nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
        break;
      case MOVIE_DECREASE_RATE:
        targetFPS -= 1.0;
        if (targetFPS < 0.0) {
          targetFPS = 0.5;
        }
        canvas->ReportRateChange(targetFPS);
        if (targetFPS < 0.2) {
          targetFPS = 0.2; 
          canvas->ReportRateChange(targetFPS);
        }
        nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
        break;
      case MOVIE_SET_RATE:
        /* User changed the frame rate slider */
        targetFPS = MAX2(event.rate, 0.2);
        /* if (targetFPS > 0.0)
           nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
           else
           nextSwapTime = 0.0;
        */
        nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
        canvas->ReportRateChange(targetFPS);
        break;
      case MOVIE_INCREASE_LOD:
        lodBias++;
        if (lodBias > maxLOD) lodBias = maxLOD; 
        canvas->ReportDetailChange(lodBias);
        break;
      case MOVIE_DECREASE_LOD:
        lodBias--;
        if (lodBias < 0) lodBias = 0; 
        canvas->ReportDetailChange(lodBias);
        break;
      case MOVIE_SET_LOD:
        lodBias = event.number;
        canvas->ReportDetailChange(lodBias);
        break;
      case MOVIE_MOUSE_PRESS_3:
        break;
      case MOVIE_SHOW_INTERFACE:
        drawInterface = true; 
        canvas->ShowInterface(true);
        break;
      case MOVIE_HIDE_INTERFACE:
        drawInterface = false; 
        canvas->ShowInterface(false);        
        break;
      case MOVIE_MOUSE_RELEASE_3:
      case MOVIE_TOGGLE_INTERFACE:
        if (!gMainWindow->isVisible()) {
          gMainWindow->show(); 
          gMainWindow->raise(); 
          gCoreApp->processEvents(); 
          gMainWindow->activateWindow(); 
        } else {
          gMainWindow->hide(); 
          gMainWindow->setVisible(false); 
        }
       break;
       
      case MOVIE_SET_STEREO:
      MOVIE_SET_STEREO:
        /* Done with the canvas */
        delete canvas;
        if (event.number) {// TURN ON STEREO
          cerr << "TURN ON STEREO" << endl; 
          if (options->rendererName == "dmx") {
            options->backendRendererName = "gl_stereo";
          } else {
            options->rendererName = "gl_stereo"; 
          }
        } else { // TURN OFF STEREO
          cerr << "TURN OFF STEREO" << endl; 
          if (options->rendererName == "dmx") {
            options->backendRendererName = "gl";
          } else {
            options->rendererName = "gl"; 
          }
        }
        options->currentFrame = frameNumber; 
        options->geometry.x = canvas->XPos; 
        options->geometry.y = canvas->YPos; 
        options->geometry.width = canvas->width; 
        options->geometry.height = canvas->height; 
        //printf ("SET_STEREO: X,Y = %d, %d\n", canvas->XPos, canvas->YPos); 
        return 1; // restarts the DisplayLoop and creates a new canvas in stereo
      case MOVIE_OPEN_FILE:   
      case MOVIE_OPEN_FILE_NOCHANGE: 
	{
	  DEBUGMSG("Got Open_File command"); 
	  QStringList filenames; 
	  filenames.append(event.mString);
	  FrameList *newFrameList = new FrameList; 
 	  if (newFrameList->LoadFrames(filenames)) {
	    canvas->mRenderer->DestroyImageCache();
	    allFrames->DeleteFrames(); 
	    delete allFrames;
	    
	    allFrames = newFrameList;
	    allFrames->GetInfo(maxWidth, maxHeight, maxDepth, maxLOD, targetFPS);
	    FrameListModified(canvas, allFrames);
	    canvas->ReportRateChange(targetFPS); 
	    
	    startFrame = 0; 
	    endFrame = allFrames->numStereoFrames()-1; 
	    frameNumber = event.number;  
	    if (event.eventType != MOVIE_OPEN_FILE_NOCHANGE) {
	      /* Compute a Shrink to Fit zoom */
	      newZoom = ComputeZoomToFit(canvas, maxWidth, maxHeight);
	      if (newZoom > 1.0) {
		newZoom = 1.0; /* don't need expanding initially */
	      }
	      /* Reset our various parameters regarding positions */
	      xOffset = yOffset = 0; 
	      panStartX = panStartY = 0; 
	      panDeltaX = panDeltaY = 0;
	      zoomStartY = 0; zoomDelta = 0;
          if (options->LOD) {
            lodBias = options->LOD;
          } else {
            lodBias = 0;
          }
	      playDirection = 0;
	      panning = 0; 
	      zoomOne = fullScreen = false; 
	      
	    }
	    canvas->ReportFrameChange(frameNumber);
	    canvas->ReportDetailRangeChange(-maxLOD, maxLOD);
	    canvas->ReportZoomChange(newZoom);
        swapBuffers = true; 
	  } else { 
	    ERROR("Could not open movie file %s", event.mString); 
        gSidecarServer->SendEvent(MovieEvent(MOVIE_STOP_ERROR, "No frames found in movie - nothing to display"));
        return 0; 
	  }
	  frameInfo =  canvas->GetFrameInfoPtr(1);
	  preloadFrames = MIN2(options->preloadFrames, static_cast<int32_t>(allFrames->numStereoFrames()));
	}
    if (!options->stereoSwitchDisable) {
      if ((allFrames->stereo && options->rendererName != "gl_stereo") ||
          (!allFrames->stereo && options->rendererName == "gl_stereo"))
        {
          cerr << "toggle stereo automatically"<< endl; 
          event.number = (!allFrames->stereo); 
          goto MOVIE_SET_STEREO;          
        }
    } 
             
    break;
      case MOVIE_SAVE_IMAGE:
        WriteImageToFile(canvas, frameNumber);
        break;
       case MOVIE_SAVE_FRAME:
         if (frameInfo ) {
           QString filename = event.mString; 
           bool ok = true; 
           if (filename == "") {          
             filename =  QInputDialog::
               getText(NULL, "Filename",
                       "Please give the image a name.",
                       QLineEdit::Normal,
                       ParentDir(QString(frameInfo->filename)),
                       &ok);
           }
           
           if (ok && !filename.isEmpty()) {
             if (! filename.endsWith(".png")) {
               filename += ".png"; 
             }
             Rectangle region; 
             region.x = region.y = 0; 
             region.width = frameInfo->width; 
             region.height = frameInfo->height; 
             Image *image = canvas->mRenderer->GetImage(frameNumber,&region,0); 
             int size[3] = {region.width, region.height, 3}; 
             int result = 
               write_png_file(filename.toAscii().data(), 
                              static_cast<unsigned char*>(image->imageData), 
                              size);
             if (result == -1) {
               ERROR("Could not write png file %s.", filename.toAscii().data()); 
             } else {
               WARNING("Successfully wrote png file %s.", filename.toAscii().data()); 
             }
           }
         }
        
         break; 
        
      default:
        WARNING("Unrecognized movie event %d ignored", event.eventType);
        break;
      } 

      //===============================================================
      /* END SWITCH on type of event */
      //===================================================================

      if (options->noscreensaver) {
        // Generate mouse click to defeat the screen saver
        double currentTime = GetCurrentTime();
        //dbprintf(3, "noscreensaverStartTime = %f, currentTime = %f\n", 
        //        noscreensaverStartTime, currentTime); 
        if (currentTime - noscreensaverStartTime > 90) {
          dbprintf(1, "Generating false mouse click to defeat screen saver.\n"); 
          canvas->mRenderer->fakeMouseClick(); 
          noscreensaverStartTime = currentTime;
        }
      }
      
      //TIMER_PRINT("end switch, frame %d", frameNumber); 
      if (frameNumber != previousFrame) {
        DEBUGMSG("frameNumber changed  to %d after switch", frameNumber); 
      }
      /*! check if we have reached the end of a cue */
      if (cuePlaying && 
          (!playDirection  || 
           (!loopCount && playDirection > 0 && cueEndFrame != -1 && frameNumber > cueEndFrame) || 
           (!loopCount && playDirection < 0 && cueEndFrame != -1 && frameNumber < cueEndFrame)) ) {
        dbprintf(2, QString("Ending cue with playDirection=%1, cueEnd=%2, frameNumber=%3\n").arg(playDirection).arg(cueEndFrame).arg(frameNumber)); 
        cuePlaying = false; 
        canvas->reportMovieCueComplete();
        gSidecarServer->SendEvent(MovieEvent(MOVIE_CUE_COMPLETE)); 
      }
      
      if (event.eventType == MOVIE_NONE && !playDirection) {
        // We have no useful work that we are doing, sleep a bit.
        usleep(sleepAmt); 
        if (sleepAmt < 100*1000)
          sleepAmt *= 2; // sleep more next time
        // dbprintf(0, "sleeping %ld\n", sleepAmt);       
        //dbprintf(5, " start back up at outer loop\n"); 
        continue;
      }
      else if (event.eventType == MOVIE_QUIT) {
        dbprintf(5, "break from the outer loop too\n"); 
        break;
      }
      else {
        // We are doing something useful -- don't sleep; allow framerate to climb. 
        // dbprintf(0, "Not sleeping because swapBuffers=%d, eventType=%d, frameNumber=%d, previousFrame=%d\n", swapBuffers, event.eventType, frameNumber, previousFrame);
        sleepAmt = 1; 
      }
  

      if (!oldPlay && playDirection) {
        /* We're starting playback now so reset counters and timers */
        recentStartTime = GetCurrentTime();
        recentFrameCount = 0;
        if (targetFPS > 0.0)
          nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
        else
          nextSwapTime = 0.0;
      }
      
     //=====================================================================
      // The frame number manipulations above may have advanced
      // or recessed the frame number beyond bounds.  We do the right thing here.
      if (frameNumber > endFrame) {    
        if (playDirection > 0) { // we're playing forward and reached the end of movie
          if (pingpong) {
            playDirection = -playDirection; 
            frameNumber = endFrame; 
          } 
          else {
            if (loopCount>0) {
              loopCount --; 
              canvas->ReportLoopBehaviorChange(loopCount);               
            }
            if (loopCount) {
              frameNumber = startFrame; 
            } else {
              frameNumber = endFrame; 
              playDirection = 0; 
            } 
          }// if not pingpong
        } else { // we are stopped, or are playing backward, either way, it's not the end of a loop, so just fix the issue, which is probably that the user asked to skip beyond the end of the movie
          frameNumber = endFrame;
        }
      }    
      if (frameNumber < startFrame) {
        if (playDirection < 0) { // we're playing backward and reached the end of movie
          if (pingpong) {
            playDirection = -playDirection; 
            frameNumber = startFrame; 
          } 
          else {
            if (loopCount > 0) {
              loopCount --; 
              canvas->ReportLoopBehaviorChange(loopCount); 
            }
            if (loopCount) {
              frameNumber = endFrame; 
            } else { // time to stop, just stick at the end and don't render
              frameNumber = startFrame; 
              playDirection = 0; 
            } 
          }// end ! pingpong
        } else { // we are stopped, or are playing forward, either way, it's not the end of a loop, so just fix the issue, which is probably that the user asked to skip beyond the start of the movie
          frameNumber = startFrame;
        }
      }
      //=====================================================================
      
      /* frameInfo = allFrames->frames[frameNumber];  */
      frameInfo =  canvas->GetFrameInfoPtr(frameNumber); /* wrapper for stereo support */
      
      if (currentZoom != newZoom) {
        currentZoom = newZoom;
        canvas->ReportZoomChange(currentZoom);
      }      
      
      QString filename("none"); 
      
      if (allFrames && static_cast<int32_t>(allFrames->numStereoFrames()) > frameNumber) {
        filename = allFrames->getFrame(frameNumber)->filename; 
      }
      int loopmsg = 0, imageHeight=0, imageWidth = 0;
      if (loopCount < 0) loopmsg = -1;
      if (loopCount > 1) loopmsg = 1;
      if (loopCount == 1 || loopCount == 0) loopmsg = 0; 
      if (allFrames->numStereoFrames()) {
        imageHeight = allFrames->getFrame(0)->height; 
        imageWidth = allFrames->getFrame(0)->width;
      }
      
      { 
        MovieSnapshot newSnapshot(event.eventType, filename, fps, targetFPS, currentZoom, lodBias, playDirection, startFrame, endFrame, allFrames->numStereoFrames(), frameNumber, loopmsg, pingpong, fullScreen, zoomOne, options->noscreensaver, canvas->height, canvas->width, canvas->XPos, canvas->YPos, imageHeight, imageWidth, -xOffset, yOffset); 
        if (sendSnapshot || newSnapshot != oldSnapshot) {
          
          canvas->reportWindowMoved(canvas->XPos, canvas->YPos); 
          canvas->reportWindowResize(canvas->width, canvas->height); 
          canvas->reportMovieMoved(xOffset, yOffset); 
          canvas->reportMovieFrameSize(imageWidth, imageHeight); 
          canvas->reportMovieDisplayedSize
            (static_cast<int>(newZoom*imageWidth), 
             static_cast<int>(newZoom*imageHeight)); 
          dbprintf(5, QString("Sending snapshot %1\n").arg(newSnapshot.humanReadableString())); 
          gSidecarServer->SendEvent
            (MovieEvent(MOVIE_SIDECAR_STATUS, newSnapshot.toString())); 
          oldSnapshot = newSnapshot; 
        }
      }
        /*
      * Compute ROI: region of the image that's visible in the window 
      */
      {
        const int imgWidth = frameInfo->width;
        const int imgHeight = frameInfo->height;
        const int winWidth = canvas->width;
        const int winHeight = canvas->height;
        int x, y;
        int imgLeft, imgRight, imgBottom, imgTop;
        
        /* (x,y) = image coordinate at center of window (canvas) */
        x = (imgWidth / 2) + (xOffset + panDeltaX);
        y = (imgHeight / 2) + (yOffset + panDeltaY);
        
        /* Compute image coordinates which correspond to the left,
         * right, top and bottom edges of the window.
         */
        imgLeft = static_cast<int>(x - (winWidth / 2) / currentZoom);
        imgRight = static_cast<int>(x + (winWidth / 2) / currentZoom);
        imgTop = static_cast<int>(y - (winHeight / 2) / currentZoom);
        imgBottom = static_cast<int>(y + (winHeight / 2) / currentZoom);
        
        /* Compute region of the image that's visible in the window and
         * its position destX,destY relative to upper-left corner of window.
         */
        /* X axis */
        roi.width = imgRight - imgLeft;
        roi.x = imgLeft;
        if (roi.x < 0) { // left edge of the image is off screen
          /* clip left */
          roi.x = 0;
          roi.width += roi.x; // this is odd. 
        }
        if (roi.x + roi.width > imgWidth) {
          /* clip right */
          roi.width = imgWidth - roi.x;
        }
        if (roi.width < 0) {
          /* check for null image */
          roi.x = roi.width = 0;
        }
        
        // placement of image on canvas:  destX
        destX = static_cast<int>(-imgLeft * currentZoom);
        if (destX < 0) {
          /* left clip */
          destX = 0;
        }
        if (destX + roi.width * currentZoom > winWidth) {
          /* right clip */
          roi.width = static_cast<int>((winWidth - destX) / currentZoom);
          if (roi.width < 0)
            roi.width = 0;
        }
        
        /* Y axis */
        roi.height = imgBottom - imgTop;
        roi.y = imgTop;
        if (roi.y < 0) {
          /* clip top */
          roi.y = 0;
          roi.height += roi.y; // huh?  it's 0! 
        }
        if (roi.y + roi.height > imgHeight) {
          /* clip bottom */
          roi.height = imgHeight - roi.y;
        }
        if (roi.height < 0) {
          /* check for null image */
          roi.y = roi.height = 0;
        }
        
        // placement of image on canvas:  destY
        destY = static_cast<int>(-imgTop * currentZoom);
        if (destY < 0) {
          /* top clip */
          destY = 0;
        }
        if (destY + roi.height * currentZoom > winHeight) {
          /* bottom clip */
          roi.height = static_cast<int>((winHeight - destY) / currentZoom);
          if (roi.height < 0)
            roi.height = 0;
        }
        
        /*
          printf("roi: %d, %d  %d x %d  dest: %d, %d\n",
          roi.x, roi.y, roi.width, roi.height, destX, destY);
        */
      }
      
      if (lodBias < 0) {
        lodBias = 0; 
      }        
      if (lodBias > frameInfo->maxLOD) {
        lodBias = maxLOD; 
      }        
      canvas->ReportDetailChange(lodBias);
      
      if (options->noAutoRes) {
        baseLOD = 0; 
      } else {
        baseLOD = LODFromZoom(currentZoom);
      }
      lod = baseLOD > lodBias? baseLOD: lodBias;
      if (lod > frameInfo->maxLOD) {
        lod = frameInfo->maxLOD;
      }
      /* Call the canvas to render the desired area of the frame.
       * Most canvases will refer to their own image caches to load
       * the image and render it.  Some will just send the
       * request "downstream".
       *
       * If we're paused or stopped, render the frame at maximum
       * level of detail, regardless of what was requested during
       * playback.  If we're playing forward or backward, then use
       * the given level of detail.
       */
      
       TIMER_PRINT("before render"); 
       canvas->Render(frameNumber, &roi, destX, destY, currentZoom, lod);
       if (!usingDmx && frameNumber != previousFrame && previousFrame >= 0) {
         if (allFrames->stereo) {
           canvas->mRenderer->mCache->ReleaseFrame(previousFrame*2); 
           canvas->mRenderer->mCache->ReleaseFrame(previousFrame*2+1); 
         } else {
           canvas->mRenderer->mCache->ReleaseFrame(previousFrame); 
         }
       }
       TIMER_PRINT("after render"); 
     
      /* Print info in upper-left corner */
      /*!
        RDC Note:  this is only done in x11 interface mode, which is going away.  But it might be useful to have a "draw letters on screen" option.  Note that it used OpenGL to do the actual lettering. 
      */ 
      /*     if (drawInterface && canvas->DrawString != NULL) {
             char str[100];
             int row = 0;
             sprintf(str, "Frame %d of %d", frameNumber + 1, allFrames->numStereoFrames());
             canvas->DrawString(row++, 0, str);
             sprintf(str, "Frame Size: %d by %d pixels",
             frameInfo->width, frameInfo->height);
             canvas->DrawString(row++, 0, str);
             sprintf(str, "Position: %d, %d",
             -(xOffset + panDeltaX), yOffset + panDeltaY);
             canvas->DrawString(row++, 0, str);
             sprintf(str, "Zoom: %5.2f  LOD: %d (%d + %d)", currentZoom, lod, baseLOD, lodBias);
             
             canvas->DrawString(row++, 0, str);
             sprintf(str, "FPS: %5.1f (target %.1f)", fps, targetFPS);
             canvas->DrawString(row++, 0, str);
             }
      */
 
       if (playDirection) {
        /* See if we need to introduce a pause to prevent exceeding
         * the target frame rate.
         */
        double delay = nextSwapTime - GetCurrentTime();
        if (delay > 0.0) {
          usedDelayCount++;
          delay *= 0.95;  /* an empirical constant */
          usleep((unsigned long) (delay * 1000.0 * 1000.0));
        }
        else {
          skippedDelayCount++;
        }
        /* Compute next targetted swap time */
        nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
       }
       dbprintf(5, "check to swap buffers\n"); 
       if ( 1 ) {
         /* ( swapBuffers == true ||
            frameNumber != previousFrame ||
            currentZoom != oldZoom ||
            oldXOffset != xOffset ||
            oldYOffset != yOffset)  {
         */
         canvas->preloadFrames = preloadFrames; 
         canvas->playDirection = playDirection;
         canvas->startFrame = startFrame; 
        canvas->endFrame = endFrame; 
        canvas->SwapBuffers();
        if (playDirection && options->speedTest) {
          cerr << "requesting speedTest of slaves" << endl; 
          canvas->DMXSpeedTest(); 
          playDirection = 0; 
        }
        /* Give the canvas a chance to preload upcoming images
         * while the display thread is displaying an image.
         * (It's too late to preload the current image; that one
         * will either have to be in the cache, or we'll load it
         * directly.)
         * THIS IS ICKY.  But it is going away when I rewrite the cache.  
         */
        int32_t i, preloadmax = preloadFrames;
        if (preloadmax > startFrame-endFrame) {
           preloadmax = startFrame-endFrame;
        }
        for (i = 1; i <= preloadmax; i++) {
          int offset = (playDirection == -1) ? -i : i;
          int frame = (frameNumber + offset);
          if (frame > endFrame) {
            frame = startFrame + (frame - endFrame);// preload for loops
          } 
          if (frame < startFrame) {
            frame = endFrame - (startFrame - frame); // for loops
          } 
          // for very short movies, this can be a problem
          DEBUGMSG("Preload frame %d", frame); 
          canvas->Preload(frame, &roi, lod);
        }
       }
       
       if (frameNumber != previousFrame) {
         DEBUGMSG("frameNumber changed to %d during non-switch logic", frameNumber); 
         canvas->ReportFrameChange(frameNumber);
       }
      previousFrame = frameNumber; 
      oldZoom = currentZoom; 
      oldXOffset = xOffset;
      oldYOffset = yOffset;

      TIMER_PRINT("after swap"); 
      /* Advance to the next frame */
      if (playDirection) {
        /* Compute next frame number (+/- 1) */
        frameNumber = frameNumber + playDirection; // let this wrap around, do not fix
        /* Update timing info */
        totalFrameCount++;
        recentFrameCount++;
        
        endClicks = times(&endTime);
        
        recentEndTime = GetCurrentTime();
        elapsedTime = recentEndTime - recentStartTime;
        if (elapsedTime >= 0.5) {
          fps = (double) recentFrameCount / elapsedTime;
          SuppressMessageDialogs(true); 
          WARNING("Frame Rate on frame %d: %g fps", frameNumber, fps);
          SuppressMessageDialogs(false); 
          /* reset timing info so we compute FPS over last 2 seconds */
          recentStartTime = GetCurrentTime();
          recentFrameCount = 0;
        }
      }
      else {
        recentFrameCount = 0;
        fps = 0.0;
      }
      canvas->reportActualFPS(fps); 
      if ( (playExit && frameNumber >= playExit)) { 
        events.push_back(MovieEvent(MOVIE_QUIT)); 
      }
      
    }
  }
  /* Finish our timing information */
  endClicks = times(&endTime);
  elapsedTime = (endClicks - startClicks) / (double) Hertz;
  userTime = (endTime.tms_utime - startTime.tms_utime) / (double) Hertz;
  systemTime = (endTime.tms_stime - startTime.tms_stime) / (double) Hertz;
#if DEBUG
  INFO(
       "%.2f frames/sec [%d frames, %.2f elapsed, %d%% user, %d%% system, %d%% blocked]",
       totalFrameCount / elapsedTime,
       totalFrameCount,
       elapsedTime,
       (int) (userTime * 100.0/elapsedTime + 0.5),
       (int) (systemTime * 100.0/elapsedTime + 0.5),
       (int) ((elapsedTime - userTime - systemTime)*100.0/elapsedTime + 0.5)
       );
  INFO("Skipped delay count: %d; used delay count: %d",
       skippedDelayCount, usedDelayCount);
#endif
  
  /* Done with the canvas */
  delete canvas;
  
  /* Done with the frames */
  allFrames->DeleteFrames(); 
  delete allFrames;
  return 0; 
}

