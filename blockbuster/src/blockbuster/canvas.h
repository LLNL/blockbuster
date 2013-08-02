#ifndef BLOCKBUSTER_CANVAS_H
#define BLOCKBUSTER_CANVAS_H

#include "events.h"
#include "xwindow.h"
#include "Renderer.h"
#include "common.h"
#include "frames.h"
#include "settings.h"

#define SCREEN_X_MARGIN 20
#define SCREEN_Y_MARGIN 40


struct Canvas {

 Canvas(qint32 parentWindowID, ProgramOptions *options, 
         BlockbusterInterface *gui=NULL);
  
  ~Canvas(); 
  
  
  public:
  /* The fundamental operation of the Renderer is to render.        This might be assigned gl_Render (gl.cpp, gl_Initialize), x11_Render (x11.cpp: x11_initialize()), or dmx_Render (dmxglue.cpp, dmx_Initialize()).  The assignment is done 
   */
  void Render(int frameNumber, const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod) {       
    mRenderer->Render(frameNumber, imageRegion, destX, destY, zoom, lod);
  } 
  
  void WriteImageToFile(int frameNumber);

  FrameInfoPtr GetFrameInfoPtr(int frameNumber);
  
  void SetFrameList(FrameListPtr frameList) {
    mRenderer->SetFrameList(frameList); 
  }
  
  void Preload(uint32_t frameNumber,
               const Rectangle *imageRegion, uint32_t levelOfDetail){
    mRenderer->Preload(frameNumber, imageRegion, levelOfDetail); 
  }

  void Preload(uint32_t frameNumber, uint32_t preloadFrames, 
               int playDirection, uint32_t minFrame, uint32_t maxFrame,
               const Rectangle *imageRegion, uint32_t levelOfDetail){
    mRenderer->Preload(frameNumber, preloadFrames, playDirection,
                       minFrame, maxFrame, imageRegion, levelOfDetail); 
  }
  
  
 
  /* notify the renderer
   * that a resize has occurred.    */
  void Resize(int newWidth, int newHeight, int camefromX) {
    mRenderer->Resize(newWidth, newHeight, camefromX);     
  }
  
  /* Only DMX uses this message; it tells a subwindow that it is 
   * supposed to move to a new location relative to its parent
   * window.  DMX uses it to rearrange subwindows after a 
   * resize event.  The "cameFromX" parameter suppresses the 
   * redundant and bug-prone generation of XMoveWindow() calls 
   * from moves that themselves originated from X. 
   */
  void Move(int newX, int newY, int cameFromX){
    mRenderer->Move(newX, newY, cameFromX); 
  }

  /**************************************************************/
  void DrawString(int row, int column, const char *str) {
    mRenderer->DrawString(row, column, str); 
  }
  
  
  /**************************************************************/
  /* Called to swap front/back buffers */
  void SwapBuffers(void) {
    mRenderer->SwapBuffers(); 
  }
  
  // DMX SPECIFIC STUFF: 
  void DMXSendHeartbeat(void);
  void DMXSpeedTest(void);
  void DMXCheckNetwork(void);
  
  /* This is called if any module wishes to report an error,
   * warning, informational, or debug message.  The modules
   * call through the SYSERROR(), ERROR(), WARNING(), INFO(), 
   * and DEBUGMSG() macros, but ultimately the message comes to here.
   */
  MovieStatus ReportMessage(const char *file, const char *function, 
                            int line, int level, 
                            const char *message);
  
  void ReportFrameListChange(const FrameListPtr frameList);
  void ReportFrameChange(int frameNumber);
  void ReportDetailRangeChange(int min, int max);
  void ReportDetailChange(int levelOfDetail);
  void ReportRateRangeChange(float minimumRate, float maximumRate);
  void ReportLoopBehaviorChange(int behavior);
  void ReportPingPongBehaviorChange(int behavior);
  void ReportRateChange(float rate);
  void ReportZoomChange(float zoom);
  void ShowInterface(int on);
  
  void reportWindowMoved(int xpos, int ypos); 
  void reportWindowResize(int x, int y); 
  void reportMovieMoved(int xpos, int ypos); 
  void reportMovieFrameSize(int x, int y); 
  void reportMovieDisplayedSize(int x, int y); 
  void reportActualFPS(double rate); 
  void reportMovieCueStart(void); 
  void reportMovieCueComplete(void); 
  void reportStatusChanged(QString status); 
  
  
  int height;
  int width;
  int screenHeight, screenWidth; /* for when sidcar wants whole screen */ 
  int XPos; 
  int YPos; 
  int depth;
  int threads;
  int cachesize;
  
  BlockbusterInterface *mBlockbusterInterface; 
  
  FrameListPtr frameList;
  Renderer *mRenderer; 
  
  int32_t playDirection, startFrame, endFrame, preloadFrames; 
  ProgramOptions *mOptions; 
} ;
#endif
