#ifndef BB_RENDERER_H
#define BB_RENDERER_H

#include "events.h"
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
#include "pure_C.h"


/* Base class for all other renderers, defining the API required */ 
// static class factory function: CreateRenderer
// Renderers: x11Renderer(broken), glRenderer, glStereoRenderer, glTextureRenderer, dmxRenderer

class Renderer {
 public:
  // ======================================================================
  /*static Renderer *CreateRenderer(ProgramOptions *opt, qint32 parentWindowID, 
    BlockbusterInterface *gui=NULL);*/
  // ======================================================================

  
  // ======================================================================
  Renderer(ProgramOptions *options, qint32 parentWindowID, BlockbusterInterface *gui = NULL): mBlockbusterInterface(gui), mParentWindow(parentWindowID), mDoStereo(false) { 
    Init(options); 
    return; 
  } 
    
  // ======================================================================
  virtual ~Renderer() {
    Close(); 
    return; 
  } 

  
  // ======================================================================
  void Init(ProgramOptions *options); 

  // ======================================================================
  void InitWindow(string displayName);

  // ======================================================================
  virtual void InitCache(int readerThreads, int maxCachedImages);

  // ======================================================================
  void BeginXWindowInit(string displayName);
  
  // ======================================================================
  virtual void BeginRendererInit(void) {
      return; 
    } 

  // ======================================================================
  virtual void FinishRendererInit(void) =0; 
  
  // ======================================================================
  virtual void DoStereo(bool s) {
    ReportStereoChange(s); 
    return; 
  }


  void FinishXWindowInit(void); 
  void SetFullScreen(bool fullscreen) ;
  void set_mwm_border(bool onoff);

  // ======================================================================
  // the following depend on whether DMX is being used: 
  virtual void DestroyImageCache(void)
  {
    mCache.reset(); // reset the boost smart pointer
    return; 
  }

  // ======================================================================
  virtual ImagePtr GetImage(uint32_t frameNumber,
                            const Rectangle *newRegion, 
                            uint32_t levelOfDetail){
    return mCache->GetImage(frameNumber, newRegion, levelOfDetail, false); 
  }
  
  
  
  // ======================================================================
  // The fundamental operation of the Renderer is to render.     
  void Render(int frameNumber, int /*previousFrame */, 
              uint32_t preloadFrames, int playDirection, 
              uint32_t startFrame, uint32_t endFrame,
              RectanglePtr imageRegion,
              int destX, int destY, float zoom, int lod) {
    if (mFrameList->mStereo) {
      mCache->PreloadHint(preloadFrames*2, playDirection, 
                          startFrame*2, endFrame*2+1);
    } else {
      mCache->PreloadHint(preloadFrames, playDirection, 
                          startFrame, endFrame);
    }
    RenderActual(frameNumber, imageRegion, destX, destY, zoom, lod); 
    
  }
  
  // ======================================================================
  // This is the actual renderer, minus the cache decorations
  virtual void RenderActual(int frameNumber,
                            RectanglePtr imageRegion,
                            int destX, int destY, float zoom, int lod) = 0; 

 // ======================================================================
  virtual void SetFrameList(FrameListPtr frameList, int readerThreads, 
                              int maxCachedImages) ;
  
  
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
  void ReportRepeatBehaviorChange(int behavior);
  void ReportPingPongBehaviorChange(int behavior);
  void ReportRateChange(float rate);
  void ReportZoomChange(float zoom);
  void ReportZoomToFitChange(bool ztf);
  void ReportStereoChange(bool stereo); 
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
  int mScreenHeight, mScreenWidth; 
  int mXPos; 
  int mYPos; 
  int mDepth;
  //int mThreads;
  //int mCacheSize;

  BlockbusterInterface *mBlockbusterInterface; 
  
  FrameListPtr mFrameList;
   
  int32_t mPlayDirection, mStartFrame, mEndFrame, mPreloadFrames; 

  // ==============================================================
  // END  stuff from Canvas 
  // ==============================================================

  // ==============================================================
  // BEGIN stuff from XWindow (all public): 
  // ==============================================================
  virtual void ChooseVisual(void){
    mVisualInfo = pureC_x11ChooseVisual(mDisplay,  mScreenNumber);
    return; 
  }

  virtual void DrawString(int row, int column, const char *str)=0;  
  virtual void SwapBuffers(void)=0;
  
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
  XVisualInfo *mVisualInfo;
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
  bool mDoStereo; 
  // ==============================================================
  // END  stuff from XWindow 
  // ==============================================================
  QString mName; 

  // ProgramOptions *mOptions; 
  Rectangle mGeometry;
  bool mDecorations, mFullScreen, mNoSmallWindows;
  string mFontName; 
  string mDisplayName; 
  uint32_t mReaderThreads, mNumCachedImages; 

  ImageCachePtr mCache; // if not using DMX
  NewImageCachePtr mNewCache; 


} ;

#endif
