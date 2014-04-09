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
#include "glRenderer.h"
#include "x11Renderer.h"
#include "dmxRenderer.h"


// ====================================================================
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



// ====================================================================
void ClampStartEndFrames(FrameListPtr allFrames, 
						 int32_t &startFrame, 
						 int32_t &endFrame, 
						 int32_t &frameNumber, 
						 bool warn = false) {
  DEBUGMSG(QString("BEGIN ClampStartEndFrames(%1, %2, %3, %4...").arg(allFrames->getFrame(0)->mFilename.c_str()).arg(startFrame).arg(endFrame).arg(frameNumber)); 
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
  DEBUGMSG(QString("END ClampStartEndFrames(%1, %2, %3, %4...").arg(allFrames->getFrame(0)->mFilename.c_str()).arg(startFrame).arg(endFrame).arg(frameNumber)); 
  return;
}

// ====================================================================
/*
 * Main UI / image display loop
 * XXX this should get moved to a new file.
 */
int DisplayLoop(ProgramOptions *options, vector<MovieEvent> script)
{
  int32_t cueEndFrame = 0;
  uint recentFrameCount = 0;
  FrameInfoPtr frameInfo;
  Renderer * renderer = NULL;
  int drawInterface = options->drawInterface;
  int skippedDelayCount = 0, usedDelayCount = 0;
  int done =0;
  /* Timing information */
  struct tms startTime, endTime;
  clock_t startClicks, endClicks;
  double recentStartTime = GetCurrentTime(), 
    recentEndTime, elapsedTime, userTime, systemTime;

  double nextSwapTime = 0.0, previousSwapTime = 0.0;
  float targetFPS = 0.0;
  FrameListPtr allFrames; 
  double noscreensaverStartTime = GetCurrentTime();


  bool cuePlaying = false;  // when true, status will not change. 
  /* UI / events */
  long sleepAmt=1; // for incremental backoff during idle
  /* Region Of Interest of the image */
  
  /* We'll need this for timing */
  long Hertz = sysconf(_SC_CLK_TCK);

  
  /* Start timing */
  startClicks = times(&startTime);
  recentStartTime = GetCurrentTime();
  
  


  deque<MovieEvent>  events; 

					
  time_t lastheartbeat = time(NULL); 
  MovieSnapshot oldSnapshot; 

  int oldPlay = 0; 

  while(! done) {
    // TIMER_PRINT("loop start"); 
    MovieEvent newEvent; 
    if (renderer)  oldPlay = renderer->mPlayDirection;
 
    /* Get an event from the renderer or network and process it.
     */   
#define DEBUG_EVENTS if (0) DEBUGMSG
    DEBUG_EVENTS("processEvents()"); 
    gCoreApp->processEvents(QEventLoop::AllEvents, 3); 
    DEBUG_EVENTS("gMainWindow->GetEvent()"); 
    if (gMainWindow->GetEvent(newEvent)) {  
      events.push_back(newEvent); 
    }
    DEBUG_EVENTS("renderer->DMXCheckNetwork()"); 
    if (renderer) renderer->DMXCheckNetwork();
    DEBUG_EVENTS("GetNetworkEvent()"); 
    if (GetNetworkEvent(&newEvent)) { /* Qt events from e.g. Sidecar */
      events.push_back(newEvent); 
    } 
    newEvent.mEventType = "MOVIE_NONE"; 
    if (renderer) {
      /* every minute or so, tell any DMX slaves we are alive */ 
      if (time(NULL) - lastheartbeat > 60) {
        renderer->DMXSendHeartbeat(); 
        lastheartbeat = time(NULL); 
      }
      DEBUG_EVENTS("renderer->GetXEvent()"); 
      renderer->GetXEvent(0, &newEvent); 
      if (renderer->mPlayDirection && newEvent.mEventType == "MOVIE_GOTO_FRAME" && 
          newEvent.mNumber == renderer->mCurrentFrame+renderer->mPlayDirection) {
        newEvent.mEventType = "MOVIE_NONE"; 
      }   
    }
    events.push_back(newEvent); 


    // we now have at least one event, even if it's a 'MOVIE_NONE'
    while (events.size()) {
      bool swapBuffers = false; 
      MovieEvent event = events[0];
      events.pop_front(); 

      bool sendSnapshot = false; 
             
      if (event.mEventType == "MOVIE_NONE") {
        //DEBUGMSG("GOT EVENT ------- %s", string(event).c_str()); 
        if (script.size()) {
          event = script[0]; 
          script.erase(script.begin()); 
        }
      }
      if (event.mEventType != "MOVIE_NONE") {        
        DEBUGMSG("GOT EVENT ------- %s", string(event).c_str()); 
        if (options->mTraceEvents) {
          if (!options->mTraceEventsFile.is_open() ) {
            if (options->mTraceEventsFilename == "") {
              options->mTraceEventsFilename = "events.log"; 
            }
            options->mTraceEventsFile.open(options->mTraceEventsFilename.toStdString().c_str()); 
          }
          if (!options->mTraceEventsFile.is_open() ) {
            dbprintf(0, "WARNMNG: Could not open tracefule %s for writing.\n", options->mTraceEventsFilename.toStdString().c_str());
            options->mTraceEvents = false; 
          } else {
            options->mTraceEventsFile << string(event) << endl; 
            options->mTraceEventsFile.flush(); 
          }        
        }
        if (event.mEventType == "MOVIE_TRACE_EVENTS") {
          if (options->mTraceEventsFile.is_open() ) {
            options->mTraceEventsFile.close(); 
          }
        
          options->mTraceEvents = event.mNumber;
          if (options->mTraceEvents) {
            if (event.mString != "") {
              options->mTraceEventsFilename = event.mString.c_str();           
            }
            options->mTraceEventsFile.open(options->mTraceEventsFilename.toStdString().c_str()); 
          }
        }
        else if (event.mEventType == "MOVIE_OPEN_FILE" ||
                 event.mEventType == "MOVIE_OPEN_FILE_NOCHANGE") {         
          DEBUGMSG("Got Open_File command"); 
          QStringList filenames; 
          filenames.append(event.mString.c_str());
          
          allFrames.reset(new FrameList); // this should delete our old FrameList
          if (!allFrames->LoadFrames(filenames)) {	    
            ERROR("Could not open movie file %s", event.mString.c_str()); 
            gSidecarServer->SendEvent(MovieEvent("MOVIE_STOP_ERROR", "No frames found in movie - nothing to display"));
            continue; 
          }
          if (!options->stereoSwitchDisable) { 
            DEBUGMSG("auto-switch stereo based on detected movie type..."); 
            if (options->rendererName != "dmx") { 
              DEBUGMSG("No DMX: switch frontend renderer to gl."); 
              options->rendererName = "gl";                        
            } else { 
              DEBUGMSG("DMX case: switch backend renderer to gl."); 
              options->backendRendererName = "gl";
            }          
          } 
          DEBUGMSG("Renderer name: %s", options->rendererName.toStdString().c_str()); 
          if (options->frameRate != 0.0) {
            targetFPS = options->frameRate; 
            options->frameRate = 0; 
          } else {
            targetFPS = allFrames->mTargetFPS;
          }
          
          
          /* Reset our various parameters regarding positions */
          if (!options->geometry.width) {
            options->geometry.width = allFrames->mWidth;
          } 
          if (!options->geometry.height) {
            options->geometry.height = allFrames->mHeight; 
          } 

          if (!renderer) {
            if (options->rendererName == "gltexture") {
              renderer = new glTextureRenderer(options, 0, gMainWindow);
            } else if (options->rendererName == "x11") {
              renderer = new x11Renderer(options, 0, gMainWindow); 
#ifdef USE_DMX
            } else if (options->rendererName == "dmx") {
              renderer = new dmxRenderer(options, 0, gMainWindow);
#endif  
            } else {
              renderer = new glRenderer(options, 0, gMainWindow); 
            }  
            if (!renderer) {
              ERROR("Could not create a renderer");
              return 1;
            }            
            renderer->InitWindow(options->displayName.toStdString()); 
            renderer->InitCache(options->readerThreads, 
                                options->mMaxCachedImages);  
            renderer->SetFullScreen(options->fullScreen);
            renderer->mZoomToFit = options->zoomToFit; 
            renderer->mPlayExit = options->playExit; 
            if (!options->zoomToFit && options->zoom != 0) {
              renderer->mZoom = options->zoom; 
            }
            renderer->mFullScreen = options->fullScreen; 
            renderer->mDecorations = options->decorations && !options->fullScreen; 
            renderer->mSizeToMovie = !options->fullScreen;
            renderer->mPlayDirection = (options->play || options->playExit)?1:0;
            renderer->mPlayExit = options->playExit; 
            
            renderer->mStartFrame = options->startFrame; 
            renderer->mCurrentFrame = options->currentFrame;    
            renderer->mEndFrame = options->endFrame;
            renderer->mPreloadFrames = options->preloadFrames; 
            renderer->mLOD = 0; 
          } 
          else { // pre-existing renderer:
            renderer->mZoomToFit = true;
            renderer->mStartFrame = renderer->mCurrentFrame = 0; 
            renderer->mEndFrame = allFrames->numStereoFrames()-1; 
            renderer->mPlayDirection = 0; 
          }   
          // reset image position and zoom:
          
          renderer->SetImageOffset(0,0); 
          
          if (options->LOD) {
            renderer->mLOD = options->LOD;
            options->LOD = 0; 
          } 
          
          if (event.mEventType != "MOVIE_OPEN_FILE_NOCHANGE") {
            renderer->mLOD = 0;
          }
          
          renderer->mNumRepeats = options->repeatCount; 
          options->repeatCount = 0; 
          
          // renderer->ShowInterface(options->drawInterface);
          renderer->DestroyImageCache();        
          renderer->SetFrameList(allFrames, options->readerThreads, 
                                 options->mMaxCachedImages);
          renderer->DoStereo(allFrames->mStereo);
          swapBuffers = true; 
          previousSwapTime = 0.0; 
          frameInfo =  renderer->GetFrameInfoPtr(0);
          renderer->mPreloadFrames = MIN2(options->preloadFrames, static_cast<int32_t>(allFrames->numStereoFrames()));
        }  // END event.mEventType == "MOVIE_OPEN_FILE") || event.mEventType == "MOVIE_OPEN_FILE_NOCHANGE" 
    
        else if (event.mEventType == "MOVIE_SET_STEREO") {
          renderer->DoStereo(event.mNumber);
          sendSnapshot = true; 
        }
        else if (event.mEventType == "MOVIE_DISABLE_DIALOGS") {   
          SuppressMessageDialogs(true); 
          continue;
        }
        else if (event.mEventType == "MOVIE_SIDECAR_STATUS" ||
                 event.mEventType == "MOVIE_SNAPSHOT" ||   
                 event.mEventType == "MOVIE_SNAPSHOT_STARTFRAME" ||   
                 event.mEventType == "MOVIE_SNAPSHOT_ENDFRAME" ||   
                 event.mEventType == "MOVIE_SNAPSHOT_ALT_ENDFRAME") {   
          sendSnapshot = true; 
        }
        else if (event.mEventType == "MOVIE_SIDECAR_BACKCHANNEL") {       
          gSidecarServer->connectToSidecar(event.mString.c_str()); 
        } //end "MOVIE_SIDECAR_BACKCHANNEL"
        else if (event.mEventType == "MOVIE_CUE_PLAY_ON_LOAD") { 
          renderer->mPlayDirection = event.mHeight; 
        } //end "MOVIE_CUE_BEGIN"
        else if (event.mEventType == "MOVIE_CUE_BEGIN") { 
          cueEndFrame = event.mNumber;  // for now, a cue will be defined to be "executing" until the end frame is reached, then the cue is complete
          if (renderer) renderer->reportMovieCueStart(); 
        } //end "MOVIE_CUE_BEGIN"
        else if (event.mEventType == "MOVIE_CUE_END") { 
          /* This event is just the end of the cue data stream, and is
             not related to when the cue is done executing.  
             In fact, the cue is not really playing until now.  
          */ 
          cuePlaying = true;
        } //end "MOVIE_CUE_END"
        /* if (event.mEventType == "MOVIE_CUE_MOVIE_NAME" || 
           event.mEventType == "MOVIE_CUE_PLAY_BACKWARD") { 
           break; */
        else if (event.mEventType == "MOVIE_PWD") {
          QString pwd =  ParentDir(QString(frameInfo->mFilename.c_str()));
          gSidecarServer->SendEvent(MovieEvent("MOVIE_PWD", pwd)); 
        } //end "MOVIE_PWD"
        else if (event.mEventType == "MOVIE_START_END_FRAMES") {
          renderer->mStartFrame = event.mWidth;
          renderer->mEndFrame = event.mHeight; 
          // apply the start and end frames to the current movie
          // clamp frame values, generate a warning if they are funky
          ClampStartEndFrames(allFrames, options->startFrame, options->endFrame, renderer->mCurrentFrame, true); 
          renderer->mStartFrame = options->startFrame;
          renderer->mEndFrame = options->endFrame;
          DEBUGMSG("START_END_FRAMES: start %d end %d current %d", renderer->mStartFrame, renderer->mEndFrame, renderer->mCurrentFrame); 
        } //end 
        else if (event.mEventType == "MOVIE_LOG_TO_FILE") {
          DEBUGMSG("MOVIE_LOG_TO_FILE event: %s", event.mString.c_str()); 
          enableLogging(true, event.mString.c_str());   
        } //end "MOVIE_START_END_FRAMES"
        else if (event.mEventType == "MOVIE_QUIT") {
          done = 1;
        } //end "MOVIE_QUIT"
        else if (event.mEventType == "MOVIE_IMAGE_MOVE") {
          renderer->SetImageOffset(-event.mX, event.mY);        
        } //end "MOVIE_IMAGE_MOVE"
        else if (event.mEventType == "MOVIE_FULLSCREEN") {
          if (renderer) {
            if (event.mNumber == 2) {
              renderer->SetFullScreen(!renderer->mFullScreen); 
            } else {
              renderer->SetFullScreen(event.mNumber); 
            }
            swapBuffers = true; 
          }
        } //end "MOVIE_FULLSCREEN"
        else if (event.mEventType == "MOVIE_SIZE_TO_MOVIE") {          
          renderer->mSizeToMovie = event.mNumber; 
          if (renderer->mSizeToMovie) {
            renderer->Resize(frameInfo->mWidth,
                             frameInfo->mHeight, 0);
            swapBuffers = true; 
          } 
        } // end "MOVIE_SIZE_TO_MOVIE"
        else if (event.mEventType == "MOVIE_MOVE" ||
                 event.mEventType == "MOVIE_RESIZE" ||
                 event.mEventType == "MOVIE_MOVE_RESIZE") {
          if (event.mEventType == "MOVIE_MOVE" || event.mEventType == "MOVIE_MOVE_RESIZE") {
            renderer->Move(event.mX, event.mY, event.mNumber);            
          }
        
          if (event.mEventType == "MOVIE_RESIZE" || event.mEventType == "MOVIE_MOVE_RESIZE") {
            renderer->Resize(event.mWidth,
                             event.mHeight, event.mNumber);
            
          }
          swapBuffers = true; 
        } // END "MOVIE_MOVE", "MOVIE_RESIZE" OR "MOVIE_MOVE_RESIZE"
        
        else if (event.mEventType == "MOVIE_SET_PINGPONG") {
          if (renderer) renderer->SetPingPong(event.mNumber); 
        } // END "MOVIE_SET_PINGPONG"
        else if (event.mEventType == "MOVIE_SET_REPEAT") {
          if (renderer) renderer->SetRepeat(event.mNumber); 
        }
        else if (event.mEventType == "MOVIE_PLAY_FORWARD") { 
          renderer->mPlayDirection = 1; 
        }
        else if (event.mEventType == "MOVIE_PLAY_BACKWARD") { 
          renderer->mPlayDirection = -1; 
        }
        else if (event.mEventType == "MOVIE_STOP") { 
          DEBUGMSG("Got stop event"); 
          renderer->mPlayDirection = 0; 
        }
        else if (event.mEventType == "MOVIE_STOP_ERROR") { 
          DEBUGMSG("Got stop error"); 
          renderer->mPlayDirection = 0; 
          gSidecarServer->SendEvent(MovieEvent("MOVIE_STOP_ERROR", event.mString.c_str()));
        }
        else  if (event.mEventType == "MOVIE_PAUSE") { 
          renderer->mPlayDirection = !renderer->mPlayDirection;
        }
        else if (event.mEventType == "MOVIE_STEP_FORWARD") { 
          renderer->AdvanceFrame(1); 
        }
        else if (event.mEventType == "MOVIE_STEP_BACKWARD") { 
          renderer->AdvanceFrame(-1); 
        }
        else if (event.mEventType == "MOVIE_SKIP_FORWARD") { 
          renderer->AdvanceFrame(20); 
        }
        else if (event.mEventType == "MOVIE_SKIP_BACKWARD") { 
          renderer->AdvanceFrame(-20); 
        }
        else if (event.mEventType == "MOVIE_SECTION_FORWARD") { 
          renderer->AdvanceFrame(MAX2(allFrames->numStereoFrames() / 4, 1));
        }
        else if (event.mEventType == "MOVIE_SECTION_BACKWARD") {
          renderer->AdvanceFrame(-MAX2(allFrames->numStereoFrames() / 4, 1));
        }
        
        else if (event.mEventType == "MOVIE_GOTO_START") {
          renderer->SetFrame(0); 
        }
        
        else if (event.mEventType == "MOVIE_GOTO_END") {
          renderer->SetFrame(-1); 
        }
        else if (event.mEventType == "MOVIE_GOTO_FRAME") {
          renderer->SetFrame(event.mNumber);
        }        
        else if (event.mEventType == "MOVIE_ZOOM_TO_FIT") { 
          dbprintf(2, "MOVIE_ZOOM_TO_FIT %d\n", event.mNumber); 
          renderer->SetZoomToFit(event.mNumber);
        }
        else if (event.mEventType == "MOVIE_ZOOM_ONE") {
          renderer->SetZoom(1.0); 
        }
        else if (event.mEventType == "MOVIE_ZOOM_SET") {
          renderer->SetZoom(event.mRate); 
        }
        else if (event.mEventType == "MOVIE_ZOOM_UP") {
          renderer->ZoomByFactor(1.2); 
        }
        else if (event.mEventType == "MOVIE_ZOOM_DOWN") {
          renderer->ZoomByFactor(0.8); 
        }
        
        else if (event.mEventType == "MOVIE_KEYBOARD_HELP") {
          PrintKeyboardControls();
        } 
        else if (event.mEventType == "MOVIE_CENTER") {
          renderer->SetImageOffset(0,0); 
        }
        else if (event.mEventType == "MOVIE_TOGGLE_CURSOR") {
          renderer->ToggleCursor(); 
        } 
        else if (event.mEventType == "MOVIE_NOSCREENSAVER") {
          options->noscreensaver = event.mNumber;
          noscreensaverStartTime = GetCurrentTime();
        } 
        else if (event.mEventType == "MOVIE_MOUSE_PRESS_1") {
          renderer->StartPanning(event.mX, event.mY); 
        }
        else if (event.mEventType == "MOVIE_MOUSE_RELEASE_1") {
          renderer->UpdatePanning(event.mX, event.mY); 
          renderer->EndPanning(); 
        }
        else if (event.mEventType == "MOVIE_MOUSE_PRESS_2") {
          renderer->StartZooming(event.mY); 
        }
        else if (event.mEventType == "MOVIE_MOUSE_RELEASE_2") {
          renderer->UpdateZooming(event.mY); 
          renderer->EndZooming(); 
        }
        else if (event.mEventType == "MOVIE_MOUSE_MOVE") {
          renderer->UpdatePanning(event.mX, event.mY); 
          renderer->UpdateZooming(event.mY); 
        }
        else if (event.mEventType == "MOVIE_INCREASE_RATE") {
          targetFPS += 1.0;
        }
        else if (event.mEventType == "MOVIE_DECREASE_RATE") {
          targetFPS -= 1.0;
          if (targetFPS < 0.0) {
            targetFPS = 0.5;
          }
          if (targetFPS < 0.2) {
            targetFPS = 0.2; 
          }
        }
        else if (event.mEventType == "MOVIE_SET_RATE") {
          /* User changed the frame rate slider */
          targetFPS = MAX2(event.mRate, 0.2);
        }
        else if (event.mEventType == "MOVIE_INCREASE_LOD") {
          if (renderer->mLOD <= renderer->mMaxLOD) {
            renderer->mLOD ++; 
          }
        }
        else if (event.mEventType == "MOVIE_DECREASE_LOD") {
          if (renderer->mLOD) {
            renderer->mLOD--;
          }
        }
        else if (event.mEventType == "MOVIE_SET_LOD") {
          renderer->mLOD = event.mNumber;
        }
        /* case "MOVIE_MOUSE_PRESS_"3:
           break;*/ 
        else if (event.mEventType == "MOVIE_SHOW_INTERFACE") {
          drawInterface = true; 
          renderer->ShowInterface(true);
        }
        else if (event.mEventType == "MOVIE_HIDE_INTERFACE") {
          drawInterface = false; 
          if (renderer) renderer->ShowInterface(false);        
        }
        else if (event.mEventType == "MOVIE_MOUSE_RELEASE_3" || 
                 event.mEventType == "MOVIE_TOGGLE_INTERFACE") {
          if (!gMainWindow->isVisible()) {
            gMainWindow->show(); 
            gMainWindow->raise(); 
            gCoreApp->processEvents(); 
            gMainWindow->activateWindow(); 
          } else {
            gMainWindow->hide(); 
            gMainWindow->setVisible(false); 
          }
        }
       
        else if (event.mEventType == "MOVIE_SAVE_IMAGE") {
          renderer->WriteImageToFile(renderer->mCurrentFrame);
        }
        else if (event.mEventType == "MOVIE_SAVE_FRAME") {
          if (frameInfo ) {
            QString filename = event.mString.c_str(); 
            bool ok = true; 
            if (filename == "") {          
              filename =  QInputDialog::
                getText(NULL, "Filename",
                        "Please give the image a name.",
                        QLineEdit::Normal,
                        ParentDir(QString(frameInfo->mFilename.c_str())),
                        &ok);
            }
           
            if (ok && !filename.isEmpty()) {
              if (! filename.endsWith(".png")) {
                filename += ".png"; 
              }
              Rectangle region; 
              region.x = region.y = 0; 
              region.width = frameInfo->mWidth; 
              region.height = frameInfo->mHeight; 
              ImagePtr image = renderer->GetImage(renderer->mCurrentFrame,&region,0); 
              int size[3] = {region.width, region.height, 3}; 
              int result = 
                write_png_file(filename.toAscii().data(), 
                               (unsigned char*)image->Data(), 
                               size);
              if (result == -1) {
                ERROR("Could not write png file %s.", filename.toAscii().data()); 
              } else {
                WARNING("Successfully wrote png file %s.", filename.toAscii().data()); 
              }
            }
          }
        
        } // END event.mEventType == "MOVIE_SAVE_FRAME"
        else {
          cerr << "Warning: unknown event: " << event.mEventType << endl; 
        }
      }

      //===============================================================
      /* END SWITCH on type of event */
      //===================================================================

      if (renderer && options->noscreensaver) {
        // Generate mouse click to defeat the screen saver
        double currentTime = GetCurrentTime();
        //dbprintf(3, "noscreensaverStartTime = %f, currentTime = %f\n", 
        //        noscreensaverStartTime, currentTime); 
        if (currentTime - noscreensaverStartTime > 90) {
          dbprintf(1, "Generating false mouse click to defeat screen saver.\n"); 
          renderer->fakeMouseClick(); 
          noscreensaverStartTime = currentTime;
        }
      }
      
      //TIMER_PRINT("end switch, frame %d", frameNumber); 
      /*! check if we have reached the end of a cue */
      if (event.mEventType == "MOVIE_NONE" && renderer && !renderer->mPlayDirection) {
        //dbprintf(5, "We have no useful work that we are doing, sleep a bit to prevent hogging CPU for no reason.  Sleeping %ld usec\n", sleepAmt); 
        usleep(sleepAmt); 
        if (sleepAmt < 100*1000)
          sleepAmt *= 2; // sleep more next time
        continue;
      }
      else if (event.mEventType == "MOVIE_QUIT") {
        DEBUGMSG("break from the outer loop too"); 
        break;
      }
      else {
        // We are doing something useful -- don't sleep; allow framerate to climb. 
        // dbprintf(0, "Not sleeping because swapBuffers=%d, eventType=%d, frameNumber=%d, swapBuffers, event.mEventType, frameNumber);
        sleepAmt = 1; 
      }
      
      
      //=====================================================================
      // The frame number manipulations above may have advanced
      // or recessed the frame number beyond bounds.  We do the right thing here.
      //=====================================================================
      
      if (allFrames && renderer) {
        /* frameInfo = allFrames->frames[frameNumber];  */
        frameInfo =  renderer->GetFrameInfoPtr(renderer->mCurrentFrame); /* wrapper for stereo support */
        
        if (cuePlaying && 
            (!renderer->mPlayDirection  || 
             (!renderer->mNumRepeats && renderer->mPlayDirection > 0 && cueEndFrame != -1 && renderer->mCurrentFrame > cueEndFrame) || 
             (!renderer->mNumRepeats && renderer->mPlayDirection < 0 && cueEndFrame != -1 && renderer->mCurrentFrame < cueEndFrame)) ) {
          dbprintf(2, QString("Ending cue with playDirection=%1, cueEnd=%2, renderer->mCurrentFrame=%3\n").arg(renderer->mPlayDirection).arg(cueEndFrame).arg(renderer->mCurrentFrame)); 
          cuePlaying = false; 
          renderer->reportMovieCueComplete();
          gSidecarServer->SendEvent(MovieEvent("MOVIE_CUE_COMPLETE")); 
        }

        renderer->ZoomToFit(); 
      
        string filename("none"); 
        
        if (static_cast<int32_t>(allFrames->numStereoFrames()) > renderer->mCurrentFrame) {
          filename = allFrames->getFrame(renderer->mCurrentFrame)->mFilename; 
        }
        
        MovieSnapshot newSnapshot(event.mEventType, filename, renderer->mFPS, targetFPS, renderer->mZoom, renderer->mLOD, renderer->mDoStereo, renderer->mPlayDirection, renderer->mStartFrame, renderer->mEndFrame, allFrames->numStereoFrames(), renderer->mCurrentFrame, renderer->mNumRepeats, renderer->mDoPingPong, renderer->mFullScreen, renderer->mSizeToMovie, renderer->mZoomToFit, options->noscreensaver, renderer->mWindowHeight, renderer->mWindowWidth, renderer->mWindowXPos, renderer->mWindowYPos, renderer->mImageHeight, renderer->mImageWidth, -renderer->mImageXOffset, renderer->mImageYOffset); 
        if (sendSnapshot || newSnapshot != oldSnapshot) {          
          
          DEBUGMSG(str(boost::format("Sending snapshot %1%") 
                       % string(newSnapshot))); 
          gSidecarServer->SendEvent
            (MovieEvent("MOVIE_SIDECAR_STATUS", string(newSnapshot).c_str())); 
          oldSnapshot = newSnapshot; 
        }
        
         
        /* This is interesting.  If you are zoomed out, you don't need
           full resolution to view the image, so only load the LOD you need.  
           This is of dubious value in the modern era.  
        */ 
        if (!options->noAutoRes) {
          uint32_t baseLOD = LODFromZoom(renderer->mZoom);
          if (baseLOD > renderer->mLOD && baseLOD < frameInfo->mLOD) {
            renderer->mLOD = baseLOD;
          }
        }
        

        /* Call the renderer to render the desired area of the frame.
         * Most rendereres will refer to their own image caches to load
         * the image and render it.  Some will just send the
         * request "downstream".
         *
         * If we're paused or stopped, render the frame at maximum
         * level of detail, regardless of what was requested during
         * playback.  If we're playing forward or backward, then use
         * the given level of detail.
         */
        
 
        DEBUGMSG("before render"); 
        renderer->Render();
        
        DEBUGMSG("after render"); 
        
       
        /* Print info in upper-left corner */
        /*!
          RDC Note:  this is only done in x11 interface mode, which is going away.  But it might be useful to have a "draw letters on screen" option.  Note that it used OpenGL to do the actual lettering. 
        */ 
        /*     if (drawInterface && renderer->DrawString != NULL) {
               char str[100];
               int row = 0;
               sprintf(str, "Frame %d of %d", renderer->mCurrentFrame + 1, allFrames->numStereoFrames());
               renderer->DrawString(row++, 0, str);
               sprintf(str, "Frame Size: %d by %d pixels",
               frameInfo->mWidth, frameInfo->mHeight);
               renderer->DrawString(row++, 0, str);
               sprintf(str, "Position: %d, %d",
               -(renderer->mImageXPos + panDeltaX), renderer->mImageYPos + panDeltaY);
               renderer->DrawString(row++, 0, str);
               sprintf(str, "Zoom: %5.2f  LOD: %d (%d + %d)", renderer->mZoom, lod, baseLOD, renderer->mLOD);
             
               renderer->DrawString(row++, 0, str);
               sprintf(str, "FPS: %5.1f (target %.1f)", fps, targetFPS);
               renderer->DrawString(row++, 0, str);
               }
        */
 
        
  
        if (renderer->mPlayDirection) {
          if (!oldPlay) {
            /* We're starting playback now so reset counters and timers */
            recentStartTime = GetCurrentTime();
            recentFrameCount = 0;
            if (targetFPS > 0.0)
              nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
            else
              nextSwapTime = 0.0;
          }
          /* See if we need to introduce a pause to prevent exceeding
           * the target frame rate.
           */
          double delay = nextSwapTime - GetCurrentTime();
          if (delay - previousSwapTime > 0.0) {
            usedDelayCount++;
            //delay *= 0.95;  /* an empirical constant */
            usleep((unsigned long) ((delay - previousSwapTime) * 1000.0 * 1000.0));
          }
          else {
            skippedDelayCount++;
          }
          /* Compute next targetted swap time */
          nextSwapTime = GetCurrentTime() + 1.0 / targetFPS;
        }
        DEBUGMSG("Swap buffers"); 
        double beforeSwap = GetCurrentTime(); 
        renderer->SwapBuffers();
        previousSwapTime = GetCurrentTime() - beforeSwap; 

        if (renderer->mPlayDirection && options->speedTest) {
          cerr << "requesting speedTest of slaves" << endl; 
          renderer->DMXSpeedTest(); 
          renderer->mPlayDirection = 0; 
        }
    

        DEBUGMSG("after swap (swap time was %0.5f ms)", previousSwapTime*1000.0); 
        /* Advance to the next frame */
        renderer->AdvanceFrame(); 
        if (renderer->mPlayDirection) {
          /* Update timing info */
          recentFrameCount++;
        
          endClicks = times(&endTime);
        
          recentEndTime = GetCurrentTime();
          elapsedTime = recentEndTime - recentStartTime;
          if (elapsedTime >= 1.0/(options->fpsSampleFrequency)) {
            renderer->mFPS = (double) recentFrameCount / elapsedTime;
            SuppressMessageDialogs(true); 
            WARNING("Frame Rate on frame %d: %g fps",
                    renderer->mCurrentFrame, renderer->mFPS);
            SuppressMessageDialogs(false); 
            /* reset timing info so we compute FPS over last 2 seconds */
            recentStartTime = GetCurrentTime();
            recentFrameCount = 0;
          }
        }
        else {
          recentFrameCount = 0;
          renderer->mFPS = 0.0;
        }
        if ( (options->playExit > 0 && renderer->mCurrentFrame >= options->playExit) ||
             (options->playExit == -1 && renderer->mCurrentFrame > renderer->mEndFrame-1)) { 
          events.push_back(MovieEvent("MOVIE_QUIT")); 
        }
        renderer->UpdateInterface(); 
      }
    }
  }
  if (renderer) delete renderer; 
  /* Finish our timing information */
  endClicks = times(&endTime);
  elapsedTime = (endClicks - startClicks) / (double) Hertz;
  userTime = (endTime.tms_utime - startTime.tms_utime) / (double) Hertz;
  systemTime = (endTime.tms_stime - startTime.tms_stime) / (double) Hertz;
  /* Done with the frames */
  return 0; 
} // end DisplayLoop

