#ifndef BB_RENDERER_H
#define BB_RENDERER_H

#include "events.h"
#include "Renderer.h"
#include "common.h"
#include "frames.h"
#include "settings.h"
#include "cache.h"
#include "ImageCache.h"

#include "QString"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "pure_C.h"


/* Base class for all other renderers, defining the API required */ 
// static class factory function: CreateRenderer
// Renderers: x11Renderer(broken), glRenderer, glStereoRenderer, glTextureRenderer, dmxRenderer

class Renderer {
 public:
  // ======================================================================
  static Renderer *CreateRenderer(ProgramOptions *options, 
                                  qint32 parentWindowID, 
                                  BlockbusterInterface *gui=NULL);
  // ======================================================================

  
  // ======================================================================
  Renderer(ProgramOptions *opt, qint32 parentWindowID, 
           BlockbusterInterface *gui, QString name="virtual");
  
  // ======================================================================
  virtual ~Renderer() {
    Close(); 
    return; 
  } 
  
  // ======================================================================
  void FinishInit(ProgramOptions *opt);
  
  // ======================================================================
  virtual void FinishRendererInit(ProgramOptions *opt) =0; 
  
  // ======================================================================
  // the following depend on whether DMX is being used: 
  virtual void DestroyImageCache(void)
  {
    mCache.reset(); 
    return; 
  }

  // ======================================================================
 virtual ImagePtr GetImage(uint32_t frameNumber,
                  const Rectangle *newRegion, uint32_t levelOfDetail){
    return mCache->GetImage(frameNumber, newRegion, levelOfDetail); 
  }

 // ======================================================================
  virtual void DecrementLockCount(ImagePtr image) {
    mCache->DecrementLockCount(image); 
  }
  
 // ======================================================================
  // The fundamental operation of the Renderer is to render.     
  void Render(int frameNumber, int previousFrame, 
              uint32_t preloadFrames, int playDirection, 
              uint32_t startFrame, uint32_t endFrame,
              RectanglePtr imageRegion,
              int destX, int destY, float zoom, int lod) {
    mCache->PreloadHint(preloadFrames, playDirection, 
                        startFrame, endFrame);

    RenderActual(frameNumber, imageRegion, destX, destY, zoom, lod); 

    if (frameNumber != previousFrame && previousFrame >= 0) {
      DecrementLockCounts(previousFrame); 
    }
  }

  // ======================================================================
  virtual void DecrementLockCounts(int frameToDecrement) {
    mCache->DecrementLockCount(frameToDecrement); 
  }

  // ======================================================================
  // This is the actual renderer, minus the cache decorations
  virtual void RenderActual(int frameNumber,
                            RectanglePtr imageRegion,
                            int destX, int destY, float zoom, int lod) = 0; 

 // ======================================================================
  virtual void SetFrameList(FrameListPtr frameList) ;
    
 // ======================================================================
  // from Canvas class 
  virtual void Preload(uint32_t frameNumber,
                       const Rectangle *imageRegion, uint32_t levelOfDetail);

 // ======================================================================
  // this calls Preload.  It's not overloaded for DMX therefore. 
  void Preload(uint32_t frameNumber, uint32_t preloadFrames, 
               int playDirection, uint32_t minFrame, uint32_t maxFrame,
               const Rectangle *imageRegion, uint32_t levelOfDetail);
  
  /* Describes best image format for the Renderer.  The various FileFormat
   * modules will be told to give us images in this format; if they
   * fail to do so, we'll convert them ourselves (an expensive but
   * functional situation).
   */
  ImageFormat mRequiredImageFormat;

  // ==============================================================
  // BEGIN stuff from Canvas: 
  // ==============================================================
  void WriteImageToFile(int frameNumber);

  FrameInfoPtr GetFrameInfoPtr(int frameNumber);

  // DMX SPECIFIC STUFF from Canvas: 
  virtual void DMXSendHeartbeat(void) {
    return; 
  }
  virtual void DMXSpeedTest(void) {
    return; 
  }
  virtual void DMXCheckNetwork(void) {
    return; 
  }

  /* This is called if any module wishes to report an error,
   * warning, informational, or debug message.  The modules
   * call through the SYSERROR(), ERROR(), WARNING(), INFO(), 
   * and DEBUGMSG() macros, but ultimately the message comes to here.
   */
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
 
 public:
  int mHeight;
  int mWidth;
  int mScreenHeight, mScreenWidth; /* for when sidcar wants whole screen */ 
  int mXPos; 
  int mYPos; 
  int mDepth;
  int mThreads;
  int mCacheSize;
  
  BlockbusterInterface *mBlockbusterInterface; 
  
  FrameListPtr mFrameList;
   
  int32_t mPlayDirection, mStartFrame, mEndFrame, mPreloadFrames; 

  // ==============================================================
  // END  stuff from Canvas 
  // ==============================================================

  // ==============================================================
  // BEGIN stuff from XWindow (all public): 
  // ==============================================================
  virtual XVisualInfo *ChooseVisual(void){
    return  pureC_x11ChooseVisual(mDisplay,  mScreenNumber);
  }
    
  void FinishXWindowInit(ProgramOptions *opt); 

  virtual void DrawString(int row, int column, const char *str)=0;  
  virtual void SwapBuffers(void)=0;
  
  void remove_mwm_border(void);
  void ShowCursor(bool show);
  void ToggleCursor(void);
  void SetTitle (QString); 

  void GetXEvent(int block, MovieEvent *movieEvent);
  void Close(void);
  void Resize(int newWidth, int newHeight, int cameFromX);
  void Move(int newX, int newY, int cameFromX);
  void fakeMouseClick(void); 

  // from WindowInfo struct:  
  Display *mDisplay;
  XVisualInfo *mVisInfo;
  int mScreenNumber;
  Window mWindow;        /* the window we're really drawing into */
  int mIsSubWindow;          /* will be true if DMX slave */
  Window mParentWindow; 
  XFontStruct *mFontInfo;
  int mFontHeight;
  Colormap mColormap;
  bool mShowCursor; 
  long mOldWidth, mOldHeight, mOldX, mOldY; 
  bool mXSync; 

  // ==============================================================
  // END  stuff from XWindow 
  // ==============================================================
  QString mName; 

 protected:   
  //  " good to have around to reduce arguments" (??)
  ProgramOptions *mOptions; 
  ImageCachePtr mCache; // if not using DMX
  NewImageCachePtr mNewCache; 


} ;

#endif
