      #ifndef BB_EVENTS_H
#define BB_EVENTS_H


#include <stdint.h>
#include <iostream>
#include "common.h"
#include "stringutil.h"
#ifdef __cplusplus
#include <QString>
#include <QStringList>
#endif

  /* Event codes */
  typedef enum {
    MOVIE_NONE = 0, /* 0 */
    MOVIE_EXPOSE,
    MOVIE_RESIZE,
    MOVIE_FULLSCREEN,  
    MOVIE_IMAGE_MOVE,  /* move image to new x,y position in canvas */
    MOVIE_MOVE,  /* 5 */ /* move canvas to new x,y position on display */
    MOVIE_MOVE_RESIZE,  /* move and resize to new x,y position */
    MOVIE_CENTER,
    MOVIE_TOGGLE_CURSOR,
    MOVIE_SET_STEREO, 

    MOVIE_ZOOM_IN = 50,
    MOVIE_ZOOM_OUT,
    MOVIE_ZOOM_FIT, 
    MOVIE_ZOOM_ONE,
    MOVIE_ZOOM_SET,
    MOVIE_ZOOM_UP, /* 55 */
    MOVIE_ZOOM_DOWN,
    MOVIE_INCREASE_LOD,
    MOVIE_DECREASE_LOD,
    MOVIE_SET_LOD,

    MOVIE_OPEN_FILE = 100, 
    MOVIE_OPEN_FILE_NOCHANGE, 
    MOVIE_PLAY_FORWARD,
    MOVIE_PLAY_BACKWARD, 
    MOVIE_SET_LOOP,
    MOVIE_SET_PINGPONG,/* 105 */ 
    MOVIE_CONTINUE,
    MOVIE_PAUSE, 
    MOVIE_STOP,
    MOVIE_STEP_FORWARD,
    MOVIE_STEP_BACKWARD, /* 110 */
    MOVIE_SKIP_FORWARD, 
    MOVIE_SKIP_BACKWARD, 
    MOVIE_SECTION_FORWARD,
    MOVIE_SECTION_BACKWARD,
    MOVIE_SLOWER,  /* 115 */ 
    MOVIE_INCREASE_RATE, 
    MOVIE_DECREASE_RATE,
    MOVIE_SET_RATE,
    MOVIE_START_END_FRAMES,
    MOVIE_GOTO_START,/* 120 */ 
    MOVIE_GOTO_END,
    MOVIE_GOTO_FRAME, 
    MOVIE_SAVE_FRAME, 

    MOVIE_MOUSE_PRESS_1 = 200,
    MOVIE_MOUSE_RELEASE_1,
    MOVIE_MOUSE_PRESS_2,
    MOVIE_MOUSE_RELEASE_2,
    MOVIE_MOUSE_PRESS_3,
    MOVIE_MOUSE_RELEASE_3, /* 205 */ 
    MOVIE_MOUSE_MOVE, 

    MOVIE_SHOW_INTERFACE = 300,
    MOVIE_HIDE_INTERFACE,
    MOVIE_TOGGLE_INTERFACE,
    MOVIE_QUIT, 
    MOVIE_STOP_ERROR,

    /* COMMUNICATIONS WITH SIDECAR */ 
    MOVIE_SIDECAR_BACKCHANNEL = 400, 
    MOVIE_SIDECAR_MESSAGE, 
    MOVIE_SIDECAR_STATUS, 
    MOVIE_SNAPSHOT,  
    MOVIE_SNAPSHOT_STARTFRAME,
    MOVIE_SNAPSHOT_ENDFRAME,  /* 405 */
    MOVIE_SNAPSHOT_ALT_ENDFRAME,
    MOVIE_PWD, 

    /* overloading "event" to also mean a field in a movie cue */
    MOVIE_CUE_BEGIN=500, 
    MOVIE_CUE_MOVIE_NAME, 
    MOVIE_CUE_PLAY_ON_LOAD, 
    MOVIE_CUE_PLAY_BACKWARD, 
    MOVIE_CUE_END, // signal end of the cue information from sidecar
    MOVIE_CUE_COMPLETE, /* 505 */ // end of the cue execution in blockbuster

    /* slave communications */
	MOVIE_SLAVE_SWAP_COMPLETE=600, // the previous SwapBuffers is done
	MOVIE_SLAVE_EXIT, // a slave is about to exit
	MOVIE_SLAVE_COMMAND_FAILED, // a command sent to a slave has failed
	MOVIE_SLAVE_ERROR, // an error has occurred in a slave
   /* ========================================================================*/
    /* This is useful if saving a splash screen image */
    MOVIE_SAVE_IMAGE,
    /* A way to force an error message to appear NOTE:  DOES NOT APPEAR TO DO ANYTHING */
    MOVIE_MESSAGE,
    /* A way to simulate the case that all or particular frames
     * fail to load
     */
    MOVIE_DISABLE_CURRENT_FRAME,
  } MovieEventType;


  uint32_t MovieEventTypeToUint32(MovieEventType iType); 
  MovieEventType Uint32ToMovieEventType (uint32_t iInt);
inline QString MovieEventTypeToString(MovieEventType iType) {
  switch (iType) {
  case MOVIE_NONE: return "MOVIE_NONE"; 
  case MOVIE_EXPOSE: return "MOVIE_EXPOSE"; 
  case MOVIE_RESIZE: return "MOVIE_RESIZE"; 
  case MOVIE_FULLSCREEN: return "MOVIE_FULLSCREEN"; 
  case MOVIE_IMAGE_MOVE: return "MOVIE_IMAGE_MOVE"; 
  case MOVIE_MOVE: return "MOVIE_MOVE"; 
  case MOVIE_MOVE_RESIZE: return "MOVIE_MOVE_RESIZE"; 
  case MOVIE_CENTER: return "MOVIE_CENTER"; 
  case MOVIE_TOGGLE_CURSOR: return "MOVIE_TOGGLE_CURSOR"; 
  case MOVIE_ZOOM_IN: return "MOVIE_ZOOM_IN"; 
  case MOVIE_ZOOM_OUT: return "MOVIE_ZOOM_OUT"; 
  case MOVIE_ZOOM_FIT: return "MOVIE_ZOOM_FIT"; 
  case MOVIE_ZOOM_ONE: return "MOVIE_ZOOM_ONE"; 
  case MOVIE_ZOOM_SET: return "MOVIE_ZOOM_SET"; 
  case MOVIE_ZOOM_UP: return "MOVIE_ZOOM_UP"; 
  case MOVIE_ZOOM_DOWN: return "MOVIE_ZOOM_DOWN"; 
  case MOVIE_INCREASE_LOD: return "MOVIE_INCREASE_LOD"; 
  case MOVIE_DECREASE_LOD: return "MOVIE_DECREASE_LOD"; 
  case MOVIE_SET_LOD: return "MOVIE_SET_LOD"; 
  case MOVIE_OPEN_FILE: return "MOVIE_OPEN_FILE"; 
  case MOVIE_OPEN_FILE_NOCHANGE: return "MOVIE_OPEN_FILE_NOCHANGE"; 
  case MOVIE_PLAY_FORWARD: return "MOVIE_PLAY_FORWARD"; 
  case MOVIE_PLAY_BACKWARD: return "MOVIE_PLAY_BACKWARD"; 
  case MOVIE_SET_LOOP: return "MOVIE_SET_LOOP"; 
  case MOVIE_SET_PINGPONG: return "MOVIE_SET_PINGPONG"; 
  case MOVIE_CONTINUE: return "MOVIE_CONTINUE"; 
  case MOVIE_PAUSE: return "MOVIE_PAUSE"; 
  case MOVIE_STOP: return "MOVIE_STOP"; 
  case MOVIE_STEP_FORWARD: return "MOVIE_STEP_FORWARD"; 
  case MOVIE_STEP_BACKWARD: return "MOVIE_STEP_BACKWARD"; 
  case MOVIE_SKIP_FORWARD: return "MOVIE_SKIP_FORWARD"; 
  case MOVIE_SKIP_BACKWARD: return "MOVIE_SKIP_BACKWARD"; 
  case MOVIE_SECTION_FORWARD: return "MOVIE_SECTION_FORWARD"; 
  case MOVIE_SECTION_BACKWARD: return "MOVIE_SECTION_BACKWARD"; 
  case MOVIE_SLOWER: return "MOVIE_SLOWER"; 
  case MOVIE_INCREASE_RATE: return "MOVIE_INCREASE_RATE"; 
  case MOVIE_DECREASE_RATE: return "MOVIE_DECREASE_RATE"; 
  case MOVIE_SET_RATE: return "MOVIE_SET_RATE"; 
  case MOVIE_START_END_FRAMES: return "MOVIE_START_END_FRAMES"; 
  case MOVIE_GOTO_START: return "MOVIE_GOTO_START"; 
  case MOVIE_GOTO_END: return "MOVIE_GOTO_END"; 
  case MOVIE_GOTO_FRAME: return "MOVIE_GOTO_FRAME"; 
  case MOVIE_SAVE_FRAME: return "MOVIE_SAVE_FRAME"; 
  case MOVIE_MOUSE_PRESS_1: return "MOVIE_MOUSE_PRESS_1"; 
  case MOVIE_MOUSE_PRESS_2: return "MOVIE_MOUSE_PRESS_2"; 
  case MOVIE_MOUSE_PRESS_3: return "MOVIE_MOUSE_PRESS_3"; 
  case MOVIE_MOUSE_RELEASE_1: return "MOVIE_MOUSE_RELEASE_1"; 
  case MOVIE_MOUSE_RELEASE_2: return "MOVIE_MOUSE_RELEASE_2"; 
  case MOVIE_MOUSE_RELEASE_3: return "MOVIE_MOUSE_RELEASE_3"; 
  case MOVIE_MOUSE_MOVE: return "MOVIE_MOUSE_MOVE"; 
  case MOVIE_SHOW_INTERFACE: return "MOVIE_SHOW_INTERFACE"; 
  case MOVIE_HIDE_INTERFACE: return "MOVIE_HIDE_INTERFACE"; 
  case MOVIE_TOGGLE_INTERFACE: return "MOVIE_TOGGLE_INTERFACE"; 
  case MOVIE_QUIT: return "MOVIE_QUIT"; 
  case MOVIE_STOP_ERROR: return "MOVIE_STOP_ERROR"; 
  case MOVIE_SIDECAR_BACKCHANNEL: return "MOVIE_SIDECAR_BACKCHANNEL"; 
  case MOVIE_SIDECAR_MESSAGE: return "MOVIE_SIDECAR_MESSAGE"; 
  case MOVIE_SIDECAR_STATUS: return "MOVIE_SIDECAR_STATUS"; 
  case MOVIE_SNAPSHOT: return "MOVIE_SNAPSHOT"; 
  case MOVIE_SNAPSHOT_STARTFRAME: return "MOVIE_SNAPSHOT_STARTFRAME"; 
  case MOVIE_SNAPSHOT_ENDFRAME: return "MOVIE_SNAPSHOT_ENDFRAME"; 
  case MOVIE_SNAPSHOT_ALT_ENDFRAME: return "MOVIE_SNAPSHOT_ALT_ENDFRAME"; 
  case MOVIE_PWD: return "MOVIE_PWD"; 
  case MOVIE_CUE_BEGIN: return "MOVIE_CUE_BEGIN"; 
  case MOVIE_CUE_MOVIE_NAME: return "MOVIE_CUE_MOVIE_NAME"; 
  case MOVIE_CUE_PLAY_ON_LOAD: return "MOVIE_CUE_PLAY_ON_LOAD"; 
  case MOVIE_CUE_PLAY_BACKWARD: return "MOVIE_CUE_PLAY_BACKWARD"; 
  case MOVIE_CUE_END: return "MOVIE_CUE_END"; 
  case MOVIE_CUE_COMPLETE: return "MOVIE_CUE_COMPLETE"; 
  case MOVIE_SLAVE_SWAP_COMPLETE: return "MOVIE_SLAVE_SWAP_COMPLETE"; 
  case MOVIE_SLAVE_EXIT: return "MOVIE_SLAVE_EXIT"; 
  case MOVIE_SLAVE_COMMAND_FAILED: return "MOVIE_SLAVE_COMMAND_FAILED"; 
  case MOVIE_SLAVE_ERROR: return "MOVIE_SLAVE_ERROR"; 
  case MOVIE_SAVE_IMAGE: return "MOVIE_SAVE_IMAGE"; 
  case MOVIE_MESSAGE: return "MOVIE_MESSAGE"; 
  case MOVIE_DISABLE_CURRENT_FRAME: return "MOVIE_DISABLE_CURRENT_FRAME"; 
  default: return QString("UNDEFINED TYPE: %1").arg(iType);
  }
  return "this is impossible";
}
  
#define NO_MOVIE_DATA 0,0,0,0,0,0,""

  /*---------------------------------*/
  struct MovieEvent {
#ifdef __cplusplus
	MovieEvent() { init(MOVIE_NONE); }
	MovieEvent(MovieEventType eventType) { 
	  init(eventType);  
	}
	MovieEvent(MovieEventType eventType, const float irate) {
	  init(eventType); 
	  rate = irate; 
	}
    MovieEvent(MovieEventType eventType, const int32_t iwidth, const int32_t iheight, const int32_t ix, const int32_t iy) {
	  init(eventType); 
      height = iheight; 
      width = iwidth; 
      x = ix; 
      y = iy; 
      return; 
	}
	MovieEvent(MovieEventType eventType, const int32_t inumber) {
	  init(eventType); 
	  number = inumber; 
	}
	MovieEvent(MovieEventType eventType, QString iString, int32_t inumber=0) {	  
	  init(eventType); 
	  strncpy( mString, static_cast<const char*>(iString.toAscii()),BLOCKBUSTER_PATH_MAX); 
      number=inumber; 
	}
	void init(MovieEventType ieventType ) {
      ID = 0; 
	  eventType = ieventType; 
	  number = 0; 
	  rate = 0; width = 0; height = 0;
	  x = 0; y = 0;
      mString[0] = 0;
	}
    std::string Stringify(void) {
      return QString("Event: type %1, ID %2, number = %3, rate = %4, (width,height) = (%5, %6), (x,y) = (%7, %8), string: \"%9\"")
        //.arg(MovieEventTypeToUint32(eventType)).arg(ID)
        .arg(MovieEventTypeToString(eventType)).arg(ID)
        .arg(number).arg(rate).arg(width).arg(height).arg(x).arg(y)
        .arg(mString).toStdString(); 
    }
    QString ToQString(void) {
      return QString("%1 %2 %3 %4 %5 %6 %7 %8 string: %9\n")
        .arg(MovieEventTypeToUint32(eventType))
        .arg(ID)
        .arg(number)
        .arg(rate)
        .arg(width)
        .arg(height)
        .arg(x)
        .arg(y)
        .arg(mString);
    }

    void FromQString(QString line) {
      if (line.endsWith("\n")) line.chop(1);

      QStringList tokens = line.split(" ");                       
      int numtokens = tokens.size(); 
      if (numtokens < 8) {
        throw QString("Bad event in FromQString(): numtokens in \"%1\"should be 8 but is %2").arg(line).arg(numtokens); 
      }
      QStringList::iterator pos = tokens.begin(); 
      eventType = Uint32ToMovieEventType(pos->toInt()); ++pos; 
      ID = pos->toInt();  ++pos; 
      number = pos->toInt();  ++pos; 
      rate = pos->toFloat();  ++pos; 
      width = pos->toInt();  ++pos; 
      height = pos->toInt();  ++pos; 
      x = pos->toInt();  ++pos; 
      y = pos->toInt();  ++pos; 
      
      if (numtokens > 9) {
        tokens = line.split("string: ");  
        if (tokens.size() < 2) {
          eventType = MOVIE_NONE; 
          throw QString("FromQString(): Event \"%1\" is missing string").arg(line); 
        }
        strncpy(mString, tokens[1].toAscii(), BLOCKBUSTER_PATH_MAX); 
      }
      return; 
    }

#endif
    MovieEventType eventType;
    uint32_t ID; 
    int32_t number;
	float rate;
    int32_t width, height;
	int32_t x, y;
	char mString[BLOCKBUSTER_PATH_MAX];
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
    mSnapshotType(MOVIE_NONE), mFrameRate(0), mTargetFPS(30.0), 
    mZoom(0.0), mLOD(0), 
    mPlayStep(0), mStartFrame(1), mEndFrame(1),  mNumFrames(0), 
    mFrameNumber(1),  mLoop(0),  mPingPong(false), 
    mFullScreen(false), mZoomOne(false), 
    mScreenHeight(0), mScreenWidth(0), mScreenXpos(0), mScreenYpos(0), 
    mImageHeight(0), mImageWidth(0), mImageXpos(0),  mImageYpos(0){ return; }
    
  MovieSnapshot(int32_t snapshotType, 
                QString filename, float frameRate, float targetFPS, 
                float zoom, uint32_t lod, 
                int32_t playStep,  int32_t startFrame, int32_t endFrame, 
                int32_t numFrames, int32_t frameNumber,  
                int32_t loop,  bool pingpong, bool fullScreen, bool zoomOne, 
                int32_t screenHeight, int32_t screenWidth, 
                int32_t screenXpos, int32_t screenYpos, 
                int32_t imageHeight, int32_t imageWidth,
                int32_t imageXpos, int32_t imageYpos):
    mSnapshotType(snapshotType), 
    mFilename(filename), mFrameRate(frameRate), mTargetFPS(targetFPS), 
    mZoom(zoom), mLOD(lod), mPlayStep(playStep), 
    mStartFrame(startFrame), mEndFrame(endFrame),  
    mNumFrames(numFrames), mFrameNumber(frameNumber), 
    mLoop(loop), mPingPong(pingpong), 
    mFullScreen(fullScreen), mZoomOne(zoomOne), 
    mScreenHeight(screenHeight), mScreenWidth(screenWidth), 
    mScreenXpos(screenXpos), mScreenYpos(screenYpos), 
    mImageHeight(imageHeight),  mImageWidth(imageWidth),
    mImageXpos(imageXpos),  mImageYpos(imageYpos) { return; }
  
  MovieSnapshot(QString iString) {
    this->fromString(iString); 
  }
  bool operator == (const MovieSnapshot &other) const; 
  bool operator != (const MovieSnapshot &other) const {
    return ! (*this == other); 
  }

  QString toString(void) const; 
  QString humanReadableString(void) const; 
  void fromString(QString iString); 
  int32_t mSnapshotType; //e.g., MOVIE_SNAPSHOT_ENDFRAME, etc.
  QString mFilename; 
  float mFrameRate, mTargetFPS, mZoom;
  int32_t mLOD, mPlayStep, mStartFrame, mEndFrame, mNumFrames, mFrameNumber, mLoop, mPingPong, mFullScreen, mZoomOne, mScreenHeight, mScreenWidth, mScreenXpos, mScreenYpos, mImageHeight, mImageWidth, mImageXpos, mImageYpos; 
};

#endif

#endif
