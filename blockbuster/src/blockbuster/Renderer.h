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
  Renderer(ProgramOptions *options, qint32 parentWindowID, BlockbusterInterface *gui = NULL): mBlockbusterInterface(gui), mBackgroundColor(3), mDoStereo(false), mParentWindow(parentWindowID) { 
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
      mDoStereo = s;     
      return; 
    }


    void FinishXWindowInit(void); 
    void SetFullScreen(bool fullscreen) ;
    void EnableDecorations(bool);

    // ======================================================================
    virtual ImagePtr GetImage(uint32_t frameNumber,
                              const Rectangle *newRegion, 
                              uint32_t levelOfDetail){
      return mCache->GetImage(frameNumber, newRegion, levelOfDetail, false); 
    }
  
    void SetRepeat(int repeats); 
    void SetPingPong(bool pingpong); 

    // ======================================================================
    void StartZooming(int32_t mouseY) ;
    void UpdateZooming(int32_t mouseY);
    void EndZooming(void);
    void ZoomByFactor(float fact);
    void SetZoom(float zoom);
    void SetZoomToFit(bool ztf);
    void ZoomToFit(void);
  
    // ======================================================================
    void StartPanning(int32_t mouseX, int32_t mouseY);
    void UpdatePanning(int32_t mouseX, int32_t mouseY);
    void EndPanning(void);
    void SetImageOffset(int32_t x, int32_t y); 
                            
    // ======================================================================
    Rectangle ComputeROI(void);
  
    // ======================================================================
    void SetPlayDirection(int direction);

    // ======================================================================
    virtual void SetFrameList(FrameListPtr frameList, int readerThreads, 
                              int maxCachedImages) ;

    
    // ======================================================================
    void Render(void);
    void Render(Rectangle roi);
    // ======================================================================

    // ======================================================================
    // This is the actual renderer, minus the cache decorations
    virtual void RenderActual(Rectangle ROI) = 0; 

    // ======================================================================
    void SetFrame(int fnum); 
    void SetStartEndFrames(int32_t start, int32_t end); 
    void AdvanceFrame(void); 
    void AdvanceFrame(int numframes); 

   
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

  
    void UpdateInterface(void);

    void ShowInterface(int on);
  
    void reportMovieCueStart(void); 
    void reportMovieCueComplete(void); 
 
 public:


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

    void Resize(int newWidth, int newHeight, int cameFromX);
    void Move(int newX, int newY, int cameFromX);
    void fakeMouseClick(void); 
    void GetXEvent(int block, MovieEvent *movieEvent);
    void Close(void);

    ImageFormat mRequiredImageFormat;

     Rectangle mStartGeometry; 
   int mScreenHeight, mScreenWidth; 
    int mWindowWidth, mWindowHeight, 
      mWindowXPos, mWindowYPos, 
      mImageDrawX, mImageDrawY; // where to draw the image into the window
    int mImageWidth, mImageHeight;

    // PANNING
    int  mImageXOffset, mImageYOffset, // offsets from previous panning
      mPanStartX, mPanStartY, mPanX, mPanY; // current pan before mouse release
    
    bool mPanning; // to avoid spurious signals

    // ZOOMING
    double mZoom, mZoomBasis; 
    int  mZoomStartY; 
    bool mZooming; 
    bool mNoAutoRes; 
    
    // FRAME RATES
    float mFPS, mTargetFPS; 
    double mNextSwapTime, mPreviousSwapTime, mFPSSampleFrequency;
    uint mRecentFrameCount;
    double mRecentStartTime, mRecentEndTime;
    uint32_t mSkippedDelayCount, mUsedDelayCount;
        
    BlockbusterInterface *mBlockbusterInterface; 
    vector<float>mBackgroundColor; 
    uint32_t mReaderThreads, mNumCachedImages; 
    
    int32_t mCurrentFrame, mStartFrame, mEndFrame, 
      mPlayDirection, mPreloadFrames; 
    int mNumRepeats; // number of times we will "wrap around" begin or end 
    bool mDoPingPong; 
    bool mPlayExit; 
    uint32_t mDepth, mLOD, mMaxLOD;
    bool mDoStereo, mZoomToFit, 
      mSizeToMovie, mFullScreen, 
      mDecorations, mNoSmallWindows;
    string mFontName; 
    string mDisplayName; 
    FrameListPtr mFrameList;                                                
 
    // from WindowInfo struct:  
     Display *mDisplay;
    XVisualInfo *mVisualInfo;
    int mScreenNumber;
    Window mWindow;        
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

    ImageCachePtr mCache; // if not using DMX
    //  NewImageCachePtr mNewCache; 


} ;

#endif
