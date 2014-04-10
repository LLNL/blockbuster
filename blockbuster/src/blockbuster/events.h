#ifndef BB_EVENTS_H
#define BB_EVENTS_H

#include <stdint.h>
#include <iostream>
#include "common.h"
#include "stringutil.h"
#include <vector>
#include <string>

#ifdef __cplusplus
#include <QString>
#include <QStringList>
#endif


//==========================================================================
/* Event codes
   These used to be enumerated ints but are now just strings -- easier to read and no reason to go through translations.
  
  
   "MOVIE_NONE") 
   "MOVIE_LOOP_INITIALIZE" 
   "MOVIE_EXPOSE" // no value
   "MOVIE_RESIZE"  // height and width
   "MOVIE_FULLSCREEN" // true/false
   "MOVIE_SIZE_TO_MOVIE" // true/false
   "MOVIE_IMAGE_MOVE"  move image to new x;y position in canvas 
   "MOVIE_MOVE"  5 move canvas to new x;y position on display 
   "MOVIE_MOVE_RESIZE"   move and resize to new x;y position 
   "MOVIE_CENTER"  //true/false
   "MOVIE_TOGGLE_CURSOR"
   "MOVIE_NOSCREENSAVER"  Stop screensaver with fake mouse clicks 
   "MOVIE_SET_STEREO" 
   "MOVIE_DISABLE_DIALOGS"  do not display alerts in GUI -- for scripting and testing 
   "MOVIE_KEYBOARD_HELP"  print the keyboard shortcuts  

   "MOVIE_ZOOM_IN") = 50;
   "MOVIE_ZOOM_OUT"
   "MOVIE_ZOOM_TO_FIT" 
   "MOVIE_ZOOM_ONE"
   "MOVIE_ZOOM_SET"
   "MOVIE_ZOOM_UP"  55 
   "MOVIE_ZOOM_DOWN"
   "MOVIE_INCREASE_LOD"
   "MOVIE_DECREASE_LOD"
   "MOVIE_SET_LOD"

   "MOVIE_OPEN_FILE") = 100; 
   "MOVIE_PLAY_FORWARD"
   "MOVIE_PLAY_BACKWARD" 
   "MOVIE_SET_REPEAT"
   "MOVIE_SET_PINGPONG" 105  
   "MOVIE_CONTINUE"
   "MOVIE_PAUSE" 
   "MOVIE_STOP"
   "MOVIE_STEP_FORWARD"
   "MOVIE_STEP_BACKWARD"  110 
   "MOVIE_SKIP_FORWARD" 
   "MOVIE_SKIP_BACKWARD" 
   "MOVIE_SECTION_FORWARD"
   "MOVIE_SECTION_BACKWARD"
   "MOVIE_SLOWER"   115  
   "MOVIE_INCREASE_RATE" 
   "MOVIE_DECREASE_RATE"
   "MOVIE_SET_RATE"
   "MOVIE_START_END_FRAMES"
   "MOVIE_GOTO_START" 120  
   "MOVIE_GOTO_END"
   "MOVIE_GOTO_FRAME" 
   "MOVIE_SAVE_FRAME" 

   "MOVIE_MOUSE_PRESS_")1 = 200;
   "MOVIE_MOUSE_RELEASE_")1;
   "MOVIE_MOUSE_PRESS_")2;
   "MOVIE_MOUSE_RELEASE_")2;
   "MOVIE_MOUSE_PRESS_")3;
   "MOVIE_MOUSE_RELEASE_")3;  205  
   "MOVIE_MOUSE_MOVE" 

   "MOVIE_SHOW_INTERFACE") = 300;
   "MOVIE_HIDE_INTERFACE"
   "MOVIE_TOGGLE_INTERFACE"
   "MOVIE_QUIT" 
   "MOVIE_STOP_ERROR"

   COMMUNICATIONS WITH SIDECAR  
   "MOVIE_SIDECAR_BACKCHANNEL") = 400; 
   "MOVIE_SIDECAR_MESSAGE" 
   "MOVIE_SIDECAR_STATUS" 
   "MOVIE_SNAPSHOT"  
   "MOVIE_SNAPSHOT_STARTFRAME"
   "MOVIE_SNAPSHOT_ENDFRAME"   405 
   "MOVIE_SNAPSHOT_ALT_ENDFRAME"
   "MOVIE_PWD" 

   overloading "event" to also mean a field in a movie cue 
   "MOVIE_CUE_BEGIN")=500; 

   "MOVIE_CUE_MOVIE_NAME" 
   "MOVIE_CUE_PLAY_ON_LOAD" 
   "MOVIE_CUE_PLAY_BACKWARD" 
   "MOVIE_CUE_END" // signal end of the cue information from sidecar
   "MOVIE_CUE_COMPLETE"  505  // end of the cue execution in blockbuster

   slave communications 
   "MOVIE_SLAVE_SWAP_COMPLETE")=600; // the previous SwapBuffers is done
   "MOVIE_SLAVE_EXIT" // a slave is about to exit
   "MOVIE_SLAVE_COMMAND_FAILED" // a command sent to a slave has failed
   "MOVIE_SLAVE_ERROR" // an error has occurred in a slave

   ========================================================================
   This is useful if saving a splash screen image 
   "MOVIE_SAVE_IMAGE"
   A way to force an error message to appear NOTE:  DOES NOT APPEAR TO DO ANYTHING 
   "MOVIE_MESSAGE"
   Save every MovieEvent in movie.cpp:movieLoop() to a file  
   "MOVIE_TRACE_EVENTS" 
   redirect all dbprintf type stuff to a file -- actually redirects cerr and cout to a file  
   "MOVIE_LOG_TO_FILE" 
   A way to simulate the case that all or particular frames
   * fail to load
       
   "MOVIE_DISABLE_CURRENT_FRAME"
*/


//==========================================================================
#define NO_MOVIE_DATA 0,0,0,0,0,0,

/*---------------------------------*/
struct MovieEvent {
#ifdef __cplusplus
  MovieEvent() { init("MOVIE_NONE"); }
  MovieEvent(string eventType) { 
    init(eventType);  
  }
  MovieEvent(string eventType, const int32_t inumber) {
    init(eventType); 
    mNumber = inumber; 
  }
  MovieEvent(string eventType, const float irate) {
    init(eventType); 
    mRate = irate; 
  }
  MovieEvent(string eventType, const int32_t iwidth, const int32_t iheight, const int32_t ix, const int32_t iy) {
    init(eventType); 
    mHeight = iheight; 
    mWidth = iwidth; 
    mX = ix; 
    mY = iy;     
    return; 
  }
  MovieEvent(string eventType, QString iString, int32_t inumber=0) {	  
    init(eventType); 
    mString = iString.toStdString(); 
    mNumber=inumber; 
  }
  void init(string ieventType ) {
    mID = 0; 
    mEventType = ieventType; 
    mNumber = 0; 
    mRate = 0; 
    mWidth = 0; 
    mHeight = 0;
    mX = 0; 
    mY = 0;
    mString = "";
    //InitEventNames(); 
  }
    
  static vector<MovieEvent> ParseEventFile(string filename); 
  static vector<MovieEvent> ParseScript(string filename); 
  bool  ParseScriptLine(string line); 

  operator string() const; 
  MovieEvent& operator << (string line);
  MovieEvent& operator << (QString line);

#endif
  string mEventType;
  uint32_t mID; 
  int32_t mNumber;
  float mRate;
  int32_t mWidth, mHeight;
  int32_t mX, mY;
  string mString;
} ;
  
#ifdef __cplusplus
#include "QDataStream"
#include "QFile"
#include "QTcpSocket"
const QIODevice &operator <<( QIODevice &iStream,   MovieEvent &event);
const QIODevice &operator >>( QIODevice &iStream,  MovieEvent &event);

//==========================================================================
/* This structure encapsulates the current blockbuster state at any moment, for capturing into sidecar interface to create a cue from */ 
struct MovieSnapshot {
  MovieSnapshot() :
    mSnapshotType("MOVIE_NONE"), mFrameRate(0), mTargetFPS(30.0), 
    mZoom(0.0), mZoomToFit(false), mLOD(0), mStereo(false), 
    mPlayStep(0), mStartFrame(1), mEndFrame(1),  mNumFrames(0), 
    mFrameNumber(1),  mRepeat(0),  mPingPong(false), 
    mFullScreen(false), mSizeToMovie(false), mNoScreensaver(false), 
    mScreenHeight(0), mScreenWidth(0), mScreenXpos(0), mScreenYpos(0), 
    mImageHeight(0), mImageWidth(0), mImageXpos(0),  mImageYpos(0){ return; }
    
  MovieSnapshot(string snapshotType, 
                string filename, float frameRate, float targetFPS, 
                float zoom, uint32_t lod, bool stereo, 
                int32_t playStep,  int32_t startFrame, int32_t endFrame, 
                int32_t numFrames, int32_t frameNumber,  
                int32_t repeat,  bool pingpong, 
                bool fullScreen, bool sizeToMovie, 
                bool zoomToFit, 
                int32_t noScreensaver, 
                int32_t screenHeight, int32_t screenWidth, 
                int32_t screenXpos, int32_t screenYpos, 
                int32_t imageHeight, int32_t imageWidth,
                int32_t imageXpos, int32_t imageYpos):
    mSnapshotType(snapshotType), 
    mFilename(filename), mFrameRate(frameRate), mTargetFPS(targetFPS), 
    mZoom(zoom), mZoomToFit(zoomToFit), 
    mLOD(lod), mStereo(stereo), mPlayStep(playStep), 
    mStartFrame(startFrame), mEndFrame(endFrame),  
    mNumFrames(numFrames), mFrameNumber(frameNumber), 
    mRepeat(repeat), mPingPong(pingpong), 
    mFullScreen(fullScreen), mSizeToMovie(sizeToMovie), 
    mNoScreensaver(noScreensaver),
    mScreenHeight(screenHeight), mScreenWidth(screenWidth), 
    mScreenXpos(screenXpos), mScreenYpos(screenYpos), 
    mImageHeight(imageHeight),  mImageWidth(imageWidth),
    mImageXpos(imageXpos),  mImageYpos(imageYpos) { return; }
  
  MovieSnapshot(std::string iString) {
    *this << (iString); 
  } 
  bool operator == (const MovieSnapshot &other) const; 
  bool operator != (const MovieSnapshot &other) const {
    return ! (*this == other); 
  }

  operator string() const; 
  MovieSnapshot &operator <<(string iString);
  MovieSnapshot &operator <<(QString iString);
 
  //QString humanReadableString(void) const; 
  //void fromString(QString iString); 
  string mSnapshotType; //e.g., "MOVIE_SNAPSHOT_ENDFRAME", etc.
  string mFilename; 
  float mFrameRate, mTargetFPS, mZoom;
  int32_t mZoomToFit, mLOD, mStereo, mPlayStep, mStartFrame, mEndFrame, mNumFrames, mFrameNumber, mRepeat, mPingPong, mFullScreen, mSizeToMovie, mNoScreensaver, mScreenHeight, mScreenWidth, mScreenXpos, mScreenYpos, mImageHeight, mImageWidth, mImageXpos, mImageYpos; 
};

#endif

#endif
