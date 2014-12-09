#include "blockbuster_qt.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/times.h>
#include "errmsg.h"
#include "settings.h"

#include <X11/extensions/XTest.h>
#include <X11/extensions/Xinerama.h>

#include "Renderer.h"
#include "glRenderer.h"
#include "x11Renderer.h"
#ifndef NO_DMX
#include "dmxRenderer.h"
#endif

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
#define SCREEN_X_MARGIN 20
#define SCREEN_Y_MARGIN 40



// ======================================================================
void Renderer::Init(ProgramOptions *options) {
  mWindowHeight = mWindowWidth = mScreenHeight = mScreenWidth = 0; 
  mWindowXPos = mWindowYPos = mDepth = 0; 
  //mThreads = mOptions->readerThreads; 
  //mCacheSize = mOptions->mMaxCachedImages; 
  mStartGeometry = options->geometry; 
  mDecorations = options->decorations; 
  mFullScreen = options->fullScreen;
  mNoSmallWindows = options->noSmallWindows; 
  mFontName = options->fontName.toStdString(); 
  mPanX = mPanY = mImageXOffset = mImageYOffset = 0; 

  mDoPingPong = mPanning = mZooming = mNoAutoRes = false; 
  mVisualInfo = NULL; 
  mFontInfo = NULL; 
  mScreenNumber = mWindow = mIsSubWindow =  mFontHeight = 0; 
  mShowCursor = true;  
  mOldWidth = mOldHeight = mOldX = mOldY = -1; 
  mBackgroundColor = options->mBackgroundColor; 
  mNextSwapTime = 0.0;
  mPreviousSwapTime = 0.0;
  mRecentFrameCount = 0;
  mRecentStartTime = GetCurrentTime();
  mSkippedDelayCount = 0;
  mUsedDelayCount = 0;  
  mFPSSampleFrequency = 2.0; // 2 samples per second 
  mXSync = false; 
}

// ======================================================================
void Renderer::InitWindow(string displayName) {
  
  BeginXWindowInit(displayName); // previously Renderer base class constructor
  BeginRendererInit(); // previously xxRenderer child class constructor
  ChooseVisual(); 
  FinishXWindowInit(); 
  FinishRendererInit(); 
  return; 
}

// ======================================================================
void Renderer::InitCache(int readerThreads, int maxCachedImages) {
  mCache.reset(new ImageCache(readerThreads,
                              maxCachedImages,
                              mRequiredImageFormat));
  mCache->HaveStereoRenderer(mDoStereo); 
  return; 
}

// ======================================================================
void Renderer::BeginXWindowInit(string displayName) {
  ECHO_FUNCTION(5); 
  // --------------------------------------
  // From XWindow:  
  mDisplay = XOpenDisplay(displayName.c_str());
  if (!mDisplay) {
    QString err("cannot open display '%1'"); 
    ERROR(err.arg(displayName.c_str()));
    return ;
  }
  
  /* If the user asked for synchronous behavior, provide it */
  if (mXSync) {
    (void) XSynchronize(mDisplay, True);
  }
  
  if (mParentWindow)
    mIsSubWindow = 1;
  
  return; 
}


// ==============================================================
void Renderer::FinishXWindowInit(void) {
  ECHO_FUNCTION(5); 
  int x=0, y=0, width, height;
  int required_x_margin, required_y_margin;
  /* Get the screen and do some sanity checks */
  Screen *screen= ScreenOfDisplay(mDisplay, 0);
  // mScreenNumber = DefaultScreen(mDisplay);
  // Screen *screen = ScreenOfDisplay(mDisplay, mScreenNumber);
  
  /* if geometry is don't care and decorations flag is off -- then set window to max screen extents */
  mScreenWidth = WidthOfScreen(screen);
  if (mStartGeometry.width != DONT_CARE) 
    width = mStartGeometry.width;
  else {
    if(mDecorations) 
      width = DEFAULT_WIDTH;
    else 
      width =  mScreenWidth;
  }
  
  mScreenHeight = HeightOfScreen(screen);
  if (mStartGeometry.height != DONT_CARE) 
    height = mStartGeometry.height;
  else {
    if(mDecorations)
      height = DEFAULT_HEIGHT;
    else
      height =  mScreenHeight; 
  }
 
  
  /* if we've turned off the window border (decoration) with the -D flag then set rquired margins to zero
     otherwise set them to the constants defined in movie.h (SCREEN_X_MARGIN ... ) */
  if (mDecorations) {
    required_x_margin = SCREEN_X_MARGIN;
    required_y_margin = SCREEN_Y_MARGIN;
  }
  else {
    required_x_margin = 0;
    required_y_margin = 0;
  }
  
  if (mFullScreen) {
    width = WidthOfScreen(screen) - required_x_margin;
    height = HeightOfScreen(screen) - required_y_margin;
  }
  else {
    if (width > WidthOfScreen(screen) - required_x_margin) {
      DEBUGMSG("requested window width %d greater than screen width %d",
               width, WidthOfScreen(screen));
      width = WidthOfScreen(screen) - required_x_margin;
    }
    if (height > HeightOfScreen(screen) - required_y_margin) {
      DEBUGMSG("requested window height %d greater than screen height %d",
               height, HeightOfScreen(screen));
      height = HeightOfScreen(screen) - required_y_margin;
    }
  }
  
  
  if (mStartGeometry.x == CENTER)
    x = (WidthOfScreen(screen) - width) / 2;
  else if (mStartGeometry.x != DONT_CARE)
    x = mStartGeometry.x;
  else
    x = 0;
  
  if (mStartGeometry.y == CENTER)
    y = (HeightOfScreen(screen) - height) / 2;
  else if (mStartGeometry.y != DONT_CARE)
    y = mStartGeometry.y;
  else
    y = 0;
 


  XSetWindowAttributes windowAttrs;
  unsigned long windowAttrsMask;
  XSizeHints sizeHints;
  const int winBorder = 0;
  if (mVisualInfo == NULL) {
    XCloseDisplay(mDisplay);
    ERROR("Could not get mVisualInfo in XWindow constructor.\n"); 
  }

  /* Set up desired window attributes */
  mColormap = XCreateColormap(mDisplay, RootWindow(mDisplay, mScreenNumber),
                              mVisualInfo->visual, AllocNone);
  windowAttrsMask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
  windowAttrs.background_pixel = 0x0;
  windowAttrs.border_pixel = 0x0;
  windowAttrs.colormap = mColormap;
  windowAttrs.event_mask = StructureNotifyMask | ExposureMask;
  
  if (!mParentWindow) {
    windowAttrs.event_mask |= (KeyPressMask | ButtonPressMask |
                               ButtonReleaseMask | ButtonMotionMask);
    mParentWindow = RootWindow(mDisplay, mScreenNumber);
  }
  printf ("XCreateWindow: X,Y = %d, %d\n", x, y); 
  windowAttrs.override_redirect = True; // does not seem to do anything. :-(

  mWindow = XCreateWindow(mDisplay, mParentWindow,
                          x, y, width, height,
                          winBorder, mVisualInfo->depth, InputOutput,
                          mVisualInfo->visual, windowAttrsMask, &windowAttrs);
  
  XWindowAttributes win_attributes;                                   
  XGetWindowAttributes(mDisplay, mWindow, &win_attributes);           
  dbprintf(5, "After XCreateWindow: override_redirect is %d\n",  (int)win_attributes.override_redirect); 
 
  DEBUGMSG("created window 0x%x", mWindow);
  
  /* Pass some information along to the window manager to size the window */
  sizeHints.flags = USSize ;
  sizeHints.width = sizeHints.base_width = width; 
  sizeHints.height = sizeHints.base_height = height; 
  if (mNoSmallWindows) {
    // WARNING:  Setting PMinSize flag sets a hard minimum.  User cannot resize window below this.  But if I don't set it, then the window does not show up larger than a single monitor unless it's fullscreen. 
    sizeHints.flags |= PMinSize;
    sizeHints.min_width = width;
    sizeHints.min_height = height;
  }

  if (mStartGeometry.x == DONT_CARE) {
    sizeHints.x = 0; 
  } else {
    sizeHints.x = mStartGeometry.x;
  }
  if (mStartGeometry.y == DONT_CARE) {
    sizeHints.y = 0; 
  } else {
    sizeHints.y = mStartGeometry.y;
  }
  sizeHints.flags |= USPosition;
    
  SetTitle("Blockbuster"); 
  XSetStandardProperties(mDisplay, mWindow, 
                         "Blockbuster", "Blockbuster", 
                         None, (char **)NULL, 0, &sizeHints);
  
  
  /* Bring it up;  */
  XMapWindow(mDisplay, mWindow);
  XSync(mDisplay, 0);
  
  /* Font for rendering status information into the rendering window.
   */
  mFontInfo = XLoadQueryFont(mDisplay, mFontName.c_str());
  if (!mFontInfo) {
    WARNING(QString ("Couldn't load font %s, trying %s").arg(mFontName.c_str()).arg(DEFAULT_X_FONT));
    
    mFontInfo = XLoadQueryFont(mDisplay,
                               DEFAULT_X_FONT);
    if (!mFontInfo) {
      ERROR("Couldn't load DEFAULT FONT %s", DEFAULT_X_FONT);
      XFree(mVisualInfo);
      XFreeColormap(mDisplay, mColormap);
      XCloseDisplay(mDisplay);
      return ;
    }
  }
  mFontHeight = mFontInfo->ascent + mFontInfo->descent;

  XGetWindowAttributes(mDisplay, mWindow, &win_attributes);   
  dbprintf(3, "New X,Y, border width is %d, %d, %d\n", x,y, win_attributes.border_width); 

  XGetWindowAttributes(mDisplay, mParentWindow, &win_attributes); 
  dbprintf(3, "ParentWindow: X,Y =  %d, %d\n", win_attributes.x, win_attributes.y);
  //SetCanvasAttributes(mWindow); 
  mWindowWidth = width;
  mWindowHeight = height;
  mWindowXPos = x; 
  mWindowYPos = y; 
  mDepth = mVisualInfo->depth;
  return; 
}// END CONSTRUCTOR for XWindow

// ==============================================================

void Renderer::SetFullScreen(bool fullscreen) {
  // Make sure the window manager does not resize to allow for menu bar

  mFullScreen = fullscreen; 
  EnableDecorations(!fullscreen  && mDecorations);
  Atom wm_state = XInternAtom(mDisplay, "_NET_WM_STATE", False);
  Atom fsAtom = XInternAtom(mDisplay, "_NET_WM_STATE_FULLSCREEN", False);
  
  if (fullscreen) {
    Screen *screen= ScreenOfDisplay(mDisplay, 0);
    mScreenWidth = WidthOfScreen(screen);
    mScreenHeight = HeightOfScreen(screen);
    XResizeWindow(mDisplay, mWindow, mScreenWidth, mScreenHeight); 
    XSync(mDisplay, 0);
  }

  XEvent xev;
  memset(&xev, 0, sizeof(xev));
  xev.type = ClientMessage;
  xev.xclient.window = mWindow;
  xev.xclient.message_type = wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = fullscreen?1:0;
  xev.xclient.data.l[1] = fsAtom;
  xev.xclient.data.l[2] = 0;
  XSendEvent (mDisplay, DefaultRootWindow(mDisplay), False,
              SubstructureRedirectMask | SubstructureNotifyMask, &xev);
  XSync(mDisplay, 0);
  
  // Make sure the resize takes place over all monitors when using Xinerama
  
  // We have to get our window layout if multiple monitors are in use.  
  int dummy1, dummy2;
  int heads=0;
  if (XineramaQueryExtension(mDisplay, &dummy1, &dummy2)) {
    if (XineramaIsActive(mDisplay)) { 
      dbprintf(1, "Display: %dx%d\n", mScreenWidth, mScreenHeight); 
      XineramaScreenInfo *p=  XineramaQueryScreens(mDisplay, &heads);
      // useful-looking code; keep around for reference:  
      if (heads>0) {
        for (int x=0; x<heads; ++x) {
          dbprintf(1, "Head %d of %d heads: %dx%d at %d,%d\n", 
                   x+1, heads, p[x].width, p[x].height, 
                   p[x].x_org, p[x].y_org);
        }
      } else cout << "XineramaQueryScreens says there aren't any" << endl;
      XFree(p);         
    }// else cout << "Xinerama not active" << endl;
  }// else cout << "No Xinerama extension" << endl;
  Atom fullmons = XInternAtom(mDisplay, "_NET_WM_FULLSCREEN_MONITORS", False);
  memset(&xev, 0, sizeof(xev));
  xev.type = ClientMessage;
  xev.xclient.window = mWindow;
  xev.xclient.message_type = fullmons;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 0; // your topmost monitor number 
  xev.xclient.data.l[1] = heads-1; // bottommost -- assuming rectangular layout
  xev.xclient.data.l[2] = 0; // leftmost 
  xev.xclient.data.l[3] = heads-1; // rightmost -- assuming rectangular layout
  xev.xclient.data.l[4] = 0; // source indication 
  
  XSendEvent (mDisplay, DefaultRootWindow(mDisplay), False,
              SubstructureRedirectMask | SubstructureNotifyMask, &xev);
  XFlush(mDisplay);
  XSync(mDisplay, 0);
  
  // end fullscreen stuff
  return;  
}
  
// ==============================================================
/*
 * Helper to remove window decorations
 */
void Renderer::EnableDecorations(bool decorate ) {
  mDecorations = decorate; 

#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
  
  typedef struct
  {
    unsigned long    flags;
    unsigned long    functions;
    unsigned long    decorations;
    long             inputMode;
    unsigned long    status;
  } PropMotifWmHints;
  
  PropMotifWmHints motif_hints;
  Atom prop, proptype;
  
  /* setup the property */
  motif_hints.flags = MWM_HINTS_DECORATIONS;
  motif_hints.decorations = mDecorations;
  
  /* get the atom for the property */
  prop = XInternAtom(mDisplay, "_MOTIF_WM_HINTS", True );
  if (!prop) {
    /* something went wrong! */
    return;
  }
  
  /* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
  proptype = prop;
  
  XChangeProperty( mDisplay, mWindow,  /* mDisplay, mWindow */
                   prop, proptype,   /* property, type */
                   32,               /* format: 32-bit datums */
                   PropModeReplace,  /* mode */
                   (unsigned char *) &motif_hints, /* data */
                   PROP_MOTIF_WM_HINTS_ELEMENTS    /* nelements */
                   );
  XSync(mDisplay, 0);
  return; 
}
// ======================================================================
void Renderer::SetRepeat(int repeat) {
  mNumRepeats = repeat;    
  if (mNumRepeats) {
    mDoPingPong = false;
  }
  return; 
}

// ======================================================================
void Renderer::SetPingPong(bool pingpong) {
  mDoPingPong = pingpong; 
  if (mDoPingPong) {
    mNumRepeats = 0; 
  }
  return; 
}

// ======================================================================
void Renderer::StartZooming(int32_t mouseY) {   
  mZoomStartY = mouseY;
  mZoomBasis = mZoom; 
  mZooming = true; 
}


// ======================================================================
void Renderer::UpdateZooming(int32_t mouseY) {   
  if (mZooming) {
    mZoom = mZoomBasis*(1+(float)(mZoomStartY - mouseY)/(mWindowHeight));
  }
  return; 
}

// ======================================================================
void Renderer::EndZooming(void) {
  mZooming = false; 
  mZoomBasis = 0; 
}
  
// ======================================================================
void Renderer::ZoomByFactor(float fact) {
  SetZoom(mZoom * fact); 
  if (mZoom < 0.005) mZoom = 0.005; 
}

// ======================================================================
void Renderer::SetZoom(float zoom) {
  mZoom = zoom; 
  if (mZoom <= 0.0) {
    ERROR("Bad zoom: %0.3f -- resetting to 1.0", mZoom); 
    mZoom = 1.0;
  } 
  mZooming = mZoomToFit = false; 
  return; 
}

// ======================================================================
void Renderer::SetZoomToFit(bool ztf) {
  mZoomToFit = ztf; 
  if (ztf) {
    EndZooming(); 
    EndPanning(); 
    mImageXOffset = mImageYOffset = 0;
  }
  return; 
}

// ======================================================================
void Renderer::ZoomToFit(void) {
  if (mZoomToFit) {
    float xZoom = (float) mWindowWidth / mImageWidth;
    float yZoom = (float) mWindowHeight / mImageHeight;
    mZoom = xZoom < yZoom ? xZoom : yZoom;
    //     DEBUGMSG("Zoom to Fit: %f", mZoom);
  }
  return;  
}

// ======================================================================
void Renderer::StartPanning(int32_t mouseX, int32_t mouseY) {   
  mZoomToFit = false; 
  mPanX = mPanY = 0; 
  mPanStartX = mouseX; 
  mPanStartY = mouseY; 
  mPanning = true; 
  return; 
}

// ======================================================================
void Renderer::UpdatePanning(int32_t mouseX, int32_t mouseY) {   
  if (mPanning) {
    mPanX = static_cast<int>((mPanStartX - mouseX) / mZoom);
    mPanY = static_cast<int>((mPanStartY - mouseY) / mZoom);
    dbprintf(2, "UpdatePanning(): Pan = %d, %d\n",  mPanX, mPanY); 
  } 
  return; 
}

// ======================================================================
void Renderer::EndPanning(void) {
  if (mPanning) {
    mImageXOffset += mPanX; 
    mImageYOffset += mPanY; 
    dbprintf(2, "EndPanning(): Offset = %d, %d\n", 
             mImageXOffset, mImageYOffset); 
  }
  mPanning = false; 
  mPanX = mPanY = 0; 
  return; 
}

// ======================================================================
void Renderer::SetImageOffset(int32_t x, int32_t y) {
  mImageXOffset = x; 
  mImageYOffset = y; 
  EndZooming(); 
  EndPanning(); 
  return; 
}

// ======================================================================
Rectangle Renderer::ComputeROI(void) {
  Rectangle roi; 
  /* (x,y) = image coordinate at center of window (renderer) */
  int x = (mImageWidth / 2) + mImageXOffset + mPanX, 
    y = (mImageHeight / 2) + mImageYOffset + mPanY;
  
  /* Compute image coordinates which correspond to the left,
   * right, top and bottom edges of the window.
   */
  int imgLeft = static_cast<int>(x - (mWindowWidth / 2) / mZoom),
    imgRight = static_cast<int>(x + (mWindowWidth / 2) / mZoom),
    imgTop = static_cast<int>(y - (mWindowHeight / 2) / mZoom),
    imgBottom = static_cast<int>(y + (mWindowHeight / 2) / mZoom);
  dbprintf(5, "imgLeft = %d, imgRight = %d,imgTop  = %d, imgBottom = %d\n",imgLeft , imgRight, imgTop, imgBottom); 
  
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
  if (roi.x + roi.width > mImageWidth) {
    /* clip right */
    roi.width = mImageWidth - roi.x;
  }
  if (roi.width < 0) {
    /* check for null image */
    roi.x = roi.width = 0;
  }
  
  // placement of image on renderer:  mImageXOffset
  mImageDrawX = static_cast<int>(-imgLeft * mZoom);
  if (mImageDrawX < 0) {
    /* left clip */
    mImageDrawX = 0;
  }
  if (mImageDrawX + roi.width * mZoom > mWindowWidth) {
    /* right clip */
    roi.width = static_cast<int>((mWindowWidth - mImageDrawX) / mZoom);
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
  if (roi.y + roi.height > mImageHeight) {
    /* clip bottom */
    roi.height = mImageHeight - roi.y;
  }
  if (roi.height < 0) {
    /* check for null image */
    roi.y = roi.height = 0;
  }
  // placement of image on renderer:  mImageYOffset
  mImageDrawY = static_cast<int>(-imgTop * mZoom);
  if (mImageDrawY < 0) {
    /* top clip */
    mImageDrawY = 0;
  }
  if (mImageDrawY + roi.height * mZoom > mWindowHeight) {
    /* bottom clip */
    roi.height = static_cast<int>((mWindowHeight - mImageDrawY ) / mZoom);
    if (roi.height < 0)
      roi.height = 0;
  }
  
  dbprintf(5, "roi: %d, %d  %d x %d  dest: %d, %d\n",
           roi.x, roi.y, roi.width, roi.height, mImageDrawX, mImageDrawY);
  
  return roi; 
}

// ======================================================================
void Renderer::SetPlayDirection(int direction) {
  if (direction && !mPlayDirection) {
    /* We're starting playback now so reset counters and timers */
    mRecentStartTime = GetCurrentTime();
    mRecentFrameCount = 0;
    if (mTargetFPS > 0.0)
      mNextSwapTime = GetCurrentTime() + 1.0 / mTargetFPS;
    else
      mNextSwapTime = 0.0;
  }
  mPlayDirection = direction; 
  return; 
}

// ======================================================================
void Renderer::SetFrameList(FrameListPtr frameList, int readerThreads, 
                            int maxCachedImages) {
  mCache.reset(new ImageCache(readerThreads, maxCachedImages, mRequiredImageFormat));  
  if (!mCache) {
    ERROR("could not recreate image cache when changing frame list");
    exit(1); 
  }
  
  mFrameList = frameList; 
  DoStereo(frameList->mStereo); 

  mCache->ManageFrameList(frameList); 
  mCache->HaveStereoRenderer(mDoStereo); 

  mMaxLOD = frameList->mLOD; 
  mLOD = 0; 

  mEndFrame = frameList->numStereoFrames()-1;
  mStartFrame = 0; 
  mCurrentFrame = 0; 

  mImageHeight = frameList->mHeight; 
  mImageWidth = frameList->mWidth; 

  mTargetFPS = frameList->mTargetFPS; 
  mPreviousSwapTime = 0.0; 

  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameRange(1, mFrameList->numStereoFrames()); 
    mBlockbusterInterface->setFrameNumber(1); 
    QString moviename = mFrameList->getFrame(0)->mFilename.c_str(); 
    if (moviename.size() > 32) moviename = QString("...") + moviename.right(32); 
    mBlockbusterInterface->setTitle(QString("Blockbuster Control (%1)").arg(moviename)); 
    SetTitle(QString("Blockbuster (%1)").arg(moviename)); 
  }

  return; 

}

// ======================================================================
void Renderer::Render(void) {
  Render(ComputeROI()); 
}

// ======================================================================
void Renderer::Render(Rectangle roi) {
  DEBUGMSG("Render begin"); 
  if (mFrameList->mStereo) {
    mCache->PreloadHint(mPreloadFrames*2, mPlayDirection, 
                        mStartFrame*2, mEndFrame*2+1);
  } else {
    mCache->PreloadHint(mPreloadFrames, mPlayDirection, 
                        mStartFrame, mEndFrame);
  }

  // Reduce LOD if we are zoomed out enough so that pixels are aliased anyway
  uint32_t saveLOD = mLOD;
  if (!mNoAutoRes) {
    uint32_t baseLOD = 0;
    double zoom = mZoom; 
    while (zoom <= 0.5 && baseLOD < mMaxLOD) {
      zoom *= 2.0;
      baseLOD++;
    }
    if (baseLOD > mLOD) {
      mLOD = baseLOD;
    }
  }
        
  RenderActual(roi); 

  mLOD = saveLOD; 

  if (mPlayDirection) {
    /* See if we need to introduce a pause to prevent exceeding
     * the target frame rate.
     */
    double delay = mNextSwapTime - GetCurrentTime();
    if (delay - mPreviousSwapTime > 0.0) {
      mUsedDelayCount++;
      usleep((unsigned long) ((delay - mPreviousSwapTime) * 1000.0 * 1000.0));
    }
    else {
      mSkippedDelayCount++;
    }
    /* Compute next targeted swap time */
    mNextSwapTime = GetCurrentTime() + 1.0 / mTargetFPS;
  }
  DEBUGMSG("Swap buffers"); 
  double beforeSwap = GetCurrentTime(); 
  SwapBuffers();
  mPreviousSwapTime = GetCurrentTime() - beforeSwap; 
  DEBUGMSG("after swap (swap time was %0.5f ms)", mPreviousSwapTime*1000.0); 

  DEBUGMSG("Render end"); 
}

// ======================================================================
void Renderer::SetFrame(int frameNum) {
  mCurrentFrame = frameNum; 

  if (mCurrentFrame > mEndFrame || mCurrentFrame < mStartFrame) {  
    dbprintf(5, "SetFrame: we've gone beyond the end or beginning of movie. \n"); 

    uint32_t wrapFrame = 0, stopFrame = mEndFrame, step = 1; 
    if (mCurrentFrame < mStartFrame) {
      wrapFrame = mEndFrame; 
      stopFrame = 0; 
      step = -1; 
    }
    
    if (mEndFrame == mStartFrame) {
      dbprintf(5, "SetFrame: there is only a single movie frame.\n"); 
      mCurrentFrame = mStartFrame; 
    }

    else if (mDoPingPong) {
      dbprintf(5, "Turning around due to ping pong behavior.\n"); 
      mCurrentFrame = stopFrame - step;
      mPlayDirection = -mPlayDirection; // may be zero if user used "step by one" button
    } 

    else if (mNumRepeats) {
      dbprintf(5, "Wrapping around since mNumRepeats is set.\n"); 
      mCurrentFrame = wrapFrame; 
      if (mNumRepeats > 0) { // -1 means "forever"
        mNumRepeats --;   
        if (mBlockbusterInterface) {
          mBlockbusterInterface->setRepeatBehavior (mNumRepeats); 
        }
      }
    } 

    else {
      dbprintf(5, "No repeats or ping pong.  Refusing to increment.\n"); 
      mCurrentFrame = stopFrame; 
      mPlayDirection = 0; 
    } 
  }
  dbprintf(5, "SetFrame(%d), new frame is %d\n", frameNum, mCurrentFrame); 
  return; 
}

// ======================================================================
void Renderer::SetStartEndFrames(int32_t start, int32_t end) {  
  mStartFrame = start;
  mEndFrame = end; 
  int32_t lastFrame = mCache->NumStereoFrames()-1; 
  if (mStartFrame < 0) {
    dbprintf(5, "mStartFrame < 0; set to 0\n"); 
    mStartFrame = 0; 
  } else if (mStartFrame > lastFrame) {
    dbprintf(5, "mStartFrame > lastFrame; set to last frame %d\n", lastFrame); 
    mStartFrame = lastFrame; 
  }
  if (mEndFrame < 0) {
    dbprintf(5, "mEndFrame < 0; set to last frame %d\n", lastFrame); 
    mEndFrame = lastFrame; 
  } 
  if (mEndFrame < mStartFrame) {
    dbprintf(5, "mEndFrame < mStartFrame, set mEndFrame = mStartFrame\n"); 
    mEndFrame = mStartFrame; 
  }

  DEBUGMSG("START_END_FRAMES: start %d end %d current %d", mStartFrame, mEndFrame, mCurrentFrame); 
  
  return ;
}

// ======================================================================
void Renderer::AdvanceFrame(void) {
  AdvanceFrame(mPlayDirection); 
  return; 
}

// ======================================================================
void Renderer::AdvanceFrame(int numframes) {
  SetFrame(mCurrentFrame + numframes);
  if (mPlayDirection) {
    /* Update timing info */
    mRecentFrameCount++;
    mRecentEndTime = GetCurrentTime();
    double elapsedTime = mRecentEndTime - mRecentStartTime;
    if (elapsedTime >= 1.0/(mFPSSampleFrequency)) {
      mFPS = (double) mRecentFrameCount / elapsedTime;
      SuppressMessageDialogs(true); 
      WARNING("Frame Rate on frame %d: %g fps",
              mCurrentFrame, mFPS);
      SuppressMessageDialogs(false); 
      /* reset timing info for next fps computation */
      mRecentStartTime = GetCurrentTime();
      mRecentFrameCount = 0;
    }
  }
  else {
    mRecentFrameCount = 0;
    mFPS = 0.0;
  }
  return; 
}
 
// ======================================================================
void Renderer::WriteImageToFile(int frameNumber)
{
  ImagePtr image;
  Rectangle region;
  FILE *f;
  register uint32_t i;
  register unsigned char *imageData;
  uint32_t localFrameNumber = 0;

  /* explicitly use localFrameNumber as a reminder that we may be dealing with stereo movies */
  if(mFrameList->mStereo) {
    localFrameNumber = frameNumber * 2;
	
  }
  else {
    localFrameNumber = frameNumber;
  }
  

  /* This particular function only works on renderers that
   * are using the Image Cache utilities.
   */
  if (mFrameList == NULL) {
	WARNING("Cannot write image to file - frame list is NULL");
	return;
  }
  if (localFrameNumber >= mFrameList->numActualFrames()) {
	WARNING("Cannot write image to file - asked for frame %d of %d",
            localFrameNumber, mFrameList->numActualFrames());
	return;
  }

  region.x = 0;
  region.y = 0;
  region.height = mFrameList->getFrame(localFrameNumber)->mHeight;
  region.width = mFrameList->getFrame(localFrameNumber)->mWidth;

  image = mCache->GetImage(localFrameNumber, &region, 0, false);

  if (!image) {
	WARNING("Cannot write image to file - image is NULL");
	return;
  }

  /* We've got an image.  Write out a description to tmp.c */
  f = fopen("tmp.c", "w");
  if (f == NULL) {
	WARNING("Cannot write image to file - cannot open tmp.c");
	return;
  }

  fprintf(f, "static FrameInfo frameInfo = {\n");
  fprintf(f, "    %d, %d, %d, /* width, height, depth */\n",
          image->width, image->height, image->imageFormat.bytesPerPixel * 8);
  fprintf(f, "    0, /* maxLOD */\n"),
    fprintf(f, "    \"SPLASH\", /* filename */\n");
  fprintf(f, "    0, /* localFrameNumber */\n");
  fprintf(f, "    NULL, /* privateData */\n");
  fprintf(f, "    1, /* enable */\n");
  fprintf(f, "    LoadSplashScreen, /* loadImageFunc */\n");
  // fprintf(f, "    NullDestroyFrameInfo,\n");
  fprintf(f, "};\n\n");

  fprintf(f, "FrameList splashScreenFrameList = {\n");
  fprintf(f, "    1, /* count */\n");
  fprintf(f, "    0.0, /* targetFPS */\n");
  fprintf(f, "    {&frameInfo}\n");
  fprintf(f, "};\n\n");

  fprintf(f, "static unsigned char imageData[%d] = {",
          image->DataSize());
  imageData = (unsigned char *)image->Data();
  for (i = 0; i < image->DataSize(); i++) {
	if (i > 0) fprintf(f, ", ");
	if (i % 10 == 0) fprintf(f, "\n    ");
	fprintf(f, "0x%02x", imageData[i]);
  }
  fprintf(f, "\n};\n\n");
  fprintf(f, "static Image splashScreen = {\n");
  fprintf(f, "    %d, %d, /* width, height */\n", 
          image->width, image->height);
  fprintf(f, "    {\n");
  fprintf(f, "        %d, /* bytesPerPixel */\n", 
          image->imageFormat.bytesPerPixel);
  fprintf(f, "        %d, /* scanlineByteMultiple */\n",
          image->imageFormat.scanlineByteMultiple);
  fprintf(f, "        %d, /* byteOrder */\n", 
          image->imageFormat.byteOrder);
  fprintf(f, "        %d, /* rowOrder */\n", 
          image->imageFormat.rowOrder);
  fprintf(f, "        %d, %d, %d, /* redShift, greenShift, blueShift */\n",
          image->imageFormat.redShift,
          image->imageFormat.greenShift,
          image->imageFormat.blueShift);
  fprintf(f, "        0x%lx, 0x%lx, 0x%lx, /* redMask, greenMask, blueMask */\n",
          image->imageFormat.redMask,
          image->imageFormat.greenMask,
          image->imageFormat.blueMask);
  fprintf(f, "    },\n");
  fprintf(f, "    {0, 0, %d, %d}, /* x, y, width, height */\n",
          image->width, image->height);
  fprintf(f, "    0, /* levelOfDetail */\n");
  fprintf(f, "    %d, /* imageDataBytes */\n", image->DataSize());
  fprintf(f, "    (void *)imageData,\n");
  fprintf(f, "};\n");
  fclose(f);
  INFO("Saved image in tmp.c");
}

//============================================================
FrameInfoPtr Renderer::GetFrameInfoPtr(int frameNumber)
{
  if(mFrameList->mStereo) {
    return mFrameList->getFrame(frameNumber * 2);
  }
  return mFrameList->getFrame(frameNumber);
}



//============================================================
void Renderer::UpdateInterface(void) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameNumber(mCurrentFrame+1); 
    mBlockbusterInterface->setLOD(mLOD); 
    mBlockbusterInterface->reportWindowMoved(mImageXOffset, mImageYOffset); 
    mBlockbusterInterface->reportWindowResize(mWindowWidth, mWindowHeight); 
    mBlockbusterInterface->SetSizeToMovieCheckBox(mWindowWidth == mImageWidth && mWindowHeight == mImageHeight); 
    mBlockbusterInterface->reportMovieMoved(mImageXOffset, mImageYOffset); 
    mBlockbusterInterface->reportMovieFrameSize(mImageWidth, mImageHeight);
    mBlockbusterInterface->reportMovieDisplayedSize(mZoom * mImageWidth, mZoom * mImageHeight); 
    mBlockbusterInterface->setPingPongBehavior(mDoPingPong); 
    mBlockbusterInterface->setRepeatBehavior (mNumRepeats); 
    mBlockbusterInterface->setStereo(mDoStereo); 
    mBlockbusterInterface->setZoom(mZoom); 
    mBlockbusterInterface->setZoomToFit(mZoomToFit); 
    mBlockbusterInterface->reportActualFPS(mFPS); 
    mBlockbusterInterface->setLODRange(0, mMaxLOD); 
  }
}

//============================================================
void Renderer::ShowInterface(int on) {
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
void Renderer::reportMovieCueStart(void){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieCueStart(); 
  }
  return ;
}

//============================================
void Renderer::reportMovieCueComplete(void){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieCueComplete(); 
  }
  return ;
}


//======================================================   
void Renderer::ShowCursor(bool show) {
  DEBUGMSG("Show cursor: %d\n", show); 
  //cerr << "Show cursor: "<< show; 
  
  if (!show) {
    /* make a blank cursor */
    Pixmap blank;
    XColor dummy;
    char data[1] = {0};
    Cursor cursor;
    
    blank = XCreateBitmapFromData (mDisplay, mWindow, data, 1, 1); 
    if(blank == None) dbprintf(0, "error: out of memory.\n");
    cursor = XCreatePixmapCursor(mDisplay, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap (mDisplay, blank);
        
    //this makes you the cursor. then set it using this function
    XDefineCursor(mDisplay,  mWindow,  cursor);
  } else {
    if (!mShowCursor) {
      XUndefineCursor(mDisplay, mWindow);
    }
  }
  mShowCursor = show; 
  return; 
}

//======================================================   
void Renderer::ToggleCursor(void) {
  ECHO_FUNCTION(5);
  ShowCursor(!mShowCursor); 
  return; 
}

//======================================================   
void Renderer::SetTitle(QString title) {
  ECHO_FUNCTION(5);
  XStoreName(mDisplay, mWindow, title.toStdString().c_str()); 
  return; 
}

// ==============================================================
void Renderer::Resize(int newWidth, int newHeight, int cameFromX){
  ECHO_FUNCTION(5);
  dbprintf(5, "Renderer::Resize(%d, %d, %d)\n", newWidth, newHeight, cameFromX); 
  if (cameFromX) {
    mWindowHeight = newHeight; 
    mWindowWidth = newWidth; 
    return; 
  }

  if (newWidth == 0) {
    newWidth = mImageWidth;
  }
  if (newHeight == 0)  {
    newHeight = mImageHeight;
  }
  if (newWidth > mScreenWidth) {
    newWidth = mScreenWidth; 
  }
  if (newHeight > mScreenHeight) {
    newHeight = mScreenHeight; 
  }

  XWindowChanges values;
  unsigned int mask;
  
  if (mWindowWidth == newWidth && mWindowHeight == newHeight) {
    dbprintf(5, "Renderer::Resize -- no change\n"); 
    return;
  }
  
  /* X windows must be at least 1x1 */
  if (newWidth == 0)
    newWidth = 1;
  if (newHeight == 0)
    newHeight = 1;
  
  
  values.width = newWidth;
  values.height = newHeight;
  mask = CWWidth | CWHeight;
  
  mWindowWidth = newWidth;
  mWindowHeight = newHeight;

  XSizeHints sizeHints;
  long supplied_hints = 0; 
  XGetWMNormalHints(mDisplay, mWindow, &sizeHints, &supplied_hints);

  if (supplied_hints | PMinSize) {
    sizeHints.height = newHeight;
    sizeHints.base_height = newHeight;
    sizeHints.min_height = newHeight;

    sizeHints.width = newWidth;
    sizeHints.base_width = newWidth;
    sizeHints.min_width = newWidth;

    dbprintf(5, "Setting hints due to PMinSize\n"); 
    sizeHints.flags = PMinSize;    
    XSetWMNormalHints(mDisplay, mWindow, &sizeHints);    
  } else {
    dbprintf(5, "PMinSize NOT detected\n"); 
  }

  dbprintf(2, "XResizeWindow(%d, %d)\n", newWidth, newHeight); 
  XResizeWindow(mDisplay, mWindow, newWidth, newHeight); 
  XSync(mDisplay, 0);
  
  return; 
}


// ===============================================================
/*
 * Move the X window to given position 
 */
void Renderer::Move(int newX, int newY, int cameFromX) {
  ECHO_FUNCTION(5);
  //cerr << "MoveXWindow"<<endl;
  if (newX != -1) {
    mWindowXPos = newX; 
  }
  if (newY != -1) {
    mWindowYPos = newY; 
  }
  if (cameFromX) {
    return; 
  }
  XMoveWindow(mDisplay, mWindow, newX, newY);
  XSync(mDisplay, 0);
}

// ===============================================================
/*!
  Added to keep the screensaver from kicking in during a movie. 
  Moves the pointer to a certain spot and clicks there.  
  Code stolen from https://gist.github.com/726474
  Written by Pioz, whoever that is.  
  Modified by Rich Cook.  Please don't sue me.  
*/ 
void Renderer::fakeMouseClick(void)
{
  int event_base; int error_base; int major_version; int minor_version;
  if (! XTestQueryExtension(mDisplay, &event_base, &error_base, &major_version, &minor_version)) { 
    dbprintf(0, "Warning:  No fake mouse movement is possible:  XTest X11 extension is not installed\n"); 
    return; 
  } else {
    dbprintf(1, "Simulating  moving the mouse by 1 pixel in X and Y using XTest extension...\n"); 
  }
  XTestFakeRelativeMotionEvent(mDisplay,  1, 1, CurrentTime);

  return; 
  /*
    KeyCode dcode = XKeysymToKeycode(mDisplay, XStringToKeysym('$'));

    XTestFakeKeyEvent(mDisplay, dcode, True, CurrentTime);
    usleep(100);
    XTestFakeKeyEvent(mDisplay, dcode, False, CurrentTime);

    return;
  */

  // =============================================================           
  //  OLD CODE BELOW:  
  XEvent event;
  
  if(mDisplay == NULL)  {
    dbprintf(0, "Warning:  fakeMouseClick() called with no active mDisplay.\n"); 
    return; 
  }
  
  memset(&event, 0x00, sizeof(event));
  
  event.type = ButtonPress;
  event.xbutton.button = Button1;
  event.xbutton.same_screen = True;
  
  XQueryPointer(mDisplay, RootWindow(mDisplay, DefaultScreen(mDisplay)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
  
  event.xbutton.subwindow = event.xbutton.window;
  
  while(event.xbutton.subwindow)	{
    event.xbutton.window = event.xbutton.subwindow;
    
    XQueryPointer(mDisplay, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
  }
  
  /*
    if(XSendEvent(mDisplay, PointerWindow, True, 0xfff, &event) == 0)  {
    dbprintf(0, "Warning: Error with XSendEvent in fakeMouseClick()\n");
    }
  */
  if(XSendEvent(mDisplay, mWindow, True, 0xfff, &event) == 0)  {
    dbprintf(0, "Warning: Error with XSendEvent in fakeMouseClick()\n");
  }

  XFlush(mDisplay);
  
  event.type = ButtonRelease;
  event.xbutton.state = 0x100;
  
  /*
    if(XSendEvent(mDisplay, PointerWindow, True, 0xfff, &event) == 0) {
    dbprintf(0, "Warning: Error with XSendEvent in fakeMouseClick()\n");
    }
  */
  if(XSendEvent(mDisplay, mWindow, True, 0xfff, &event) == 0) {
    dbprintf(0, "Warning: Error with XSendEvent in fakeMouseClick()\n");
  }

  return; 
}
/*
 * Helper macro used by the renderers
 */
#define EVENT_TYPE_STRING(x) (                                      \
                              x==MapNotify?"MapNotify":             \
                              x==UnmapNotify?"UnmapNotify":         \
                              x==ReparentNotify?"ReparentNotify":   \
                              "unknown")


/*
 * Get the next event from the given Canvas's X window, or
 * the idle event ("MOVIE_NONE") if no event is present and
 * we aren't supposed to block waiting for input.
 */
void Renderer::GetXEvent(int block, MovieEvent *movieEvent)
{
  bool resize=false, move=false; 
  XEvent event;
  Display *dpy = mDisplay;
  if (!block && !XPending(dpy)) {
    movieEvent->mEventType = "MOVIE_NONE";
    return;
  }
  
  XNextEvent(dpy, &event);
  if (event.xconfigure.y == 22) {
    event.xconfigure.y = 22;
  }
  switch(event.type) {
  case Expose:
    while (XPending(dpy)) {
      XPeekEvent(dpy, &event);
      if (event.type == Expose) {
        XNextEvent(dpy, &event);
      }
      else
        break;
    }
    movieEvent->mEventType = "MOVIE_EXPOSE";
    return;
  case ConfigureNotify:
    while (XPending(dpy)) {
      XEvent nextEvent;
      XPeekEvent(dpy, &nextEvent);
      if (nextEvent.type == ConfigureNotify) {
        event = nextEvent;
        XNextEvent(dpy, &nextEvent);
      }
      else if (nextEvent.type == Expose) {
        XNextEvent(dpy, &nextEvent);
      }
      else
        break;
    }
    movieEvent->mWidth = event.xconfigure.width;
    movieEvent->mHeight = event.xconfigure.height;
    movieEvent->mX = event.xconfigure.x;
    movieEvent->mY = event.xconfigure.y;
    movieEvent->mNumber = 1; // means "I got this from X, so don't call XMoveResize again" -- to prevent certain loops from happening, especially on the Mac where the window will keep marching down the screen.  Other window managers might have similar pathologies, so I did not implement a Mac-specific hack. 
    
    // Check if the size really changed.  If it did, send the event.
    // Suppress it otherwise.
    if (mOldHeight != event.xconfigure.height || 
        mOldWidth != event.xconfigure.width) {
      resize = true; 
      mOldHeight = event.xconfigure.height;
      mOldWidth = event.xconfigure.width;
      movieEvent->mEventType = "MOVIE_RESIZE";
    }
    
    if (mOldX != event.xconfigure.x || mOldY != event.xconfigure.y) {
      move = true; 
      mOldX = event.xconfigure.x;
      mOldY = event.xconfigure.y;
      movieEvent->mEventType = "MOVIE_MOVE";
    }
    
    
    /* do not process these events here, as they are spurious
       if (resize && move)  movieEvent->mEventType = "MOVIE_MOVE_RESIZE";
       else*/
    if (resize)  movieEvent->mEventType = "MOVIE_RESIZE";
    else if (move)  movieEvent->mEventType = "MOVIE_MOVE";
    else movieEvent->mEventType = "MOVIE_NONE";
    
    return;
  case KeyPress:
    {
      int code = XLookupKeysym(&event.xkey, 0);
      if (code == XK_Left && event.xkey.state & ControlMask) {
        movieEvent->mEventType = "MOVIE_SECTION_BACKWARD";
        return;
      }
      if (code == XK_Left && event.xkey.state & ShiftMask) {
        movieEvent->mEventType = "MOVIE_SKIP_BACKWARD";
        return;
      }
      else if (code == XK_Left) {
        movieEvent->mEventType = "MOVIE_STEP_BACKWARD";
        return;
      }
      else if (code == XK_Right && event.xkey.state & ControlMask) {
        movieEvent->mEventType = "MOVIE_SECTION_FORWARD";
        return;
      }
      else if (code == XK_Right && event.xkey.state & ShiftMask) {
        movieEvent->mEventType = "MOVIE_SKIP_FORWARD";
        return;
      }
      else if (code == XK_Right) {
        movieEvent->mEventType = "MOVIE_STEP_FORWARD";
        return;
      }
      else if (code == XK_Home) {
        movieEvent->mEventType = "MOVIE_GOTO_START";
        return;
      }
      else if (code == XK_Escape) {
        movieEvent->mEventType = "MOVIE_QUIT";
        return;
      }  
      else if (code == XK_End) {
        movieEvent->mEventType = "MOVIE_GOTO_END";
        return;
      }
      else {
        char buffer[10];
        int r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                              NULL, NULL);
        if (r == 1) {
          switch (buffer[0]) {
          case 27:
          case 'c':
            movieEvent->mEventType = "MOVIE_CENTER";
            return;
          case 'f':
            movieEvent->mEventType = "MOVIE_ZOOM_TO_FIT";
            return;
          case 'h':
          case '?':
            movieEvent->mEventType = "MOVIE_KEYBOARD_HELP";
            break;
          case 'i':
            movieEvent->mEventType = "MOVIE_TOGGLE_INTERFACE";
            return;
          case 'l':
            movieEvent->mEventType = "MOVIE_INCREASE_LOD";
            return;
          case 'L':
            movieEvent->mEventType = "MOVIE_DECREASE_LOD";
            return;
          case 'm':
            movieEvent->mEventType = "MOVIE_TOGGLE_CURSOR"; 
            break; 
          case 'p':
            movieEvent->mEventType = "MOVIE_PLAY_FORWARD";
            return;
          case 'q':
            movieEvent->mEventType = "MOVIE_QUIT";
            return;
          case 'r':
            movieEvent->mEventType = "MOVIE_PLAY_BACKWARD";
            return;
          case 'z':
            movieEvent->mEventType = "MOVIE_ZOOM_UP";
            return;
          case 'Z':
            movieEvent->mEventType = "MOVIE_ZOOM_DOWN";
            return;
          case ' ':
            movieEvent->mEventType = "MOVIE_PAUSE";
            return;
          case '+':
            movieEvent->mEventType = "MOVIE_INCREASE_RATE";
            return;
          case '-':
            movieEvent->mEventType = "MOVIE_DECREASE_RATE";
            return;
          case '1':
            movieEvent->mEventType = "MOVIE_ZOOM_ONE";
            return;
          case '2':
            movieEvent->mEventType = "MOVIE_ZOOM_SET";
            movieEvent->mRate = 2.0f; 
            return;
          case '@': // shift-2 = '@' =  1/2
            movieEvent->mEventType = "MOVIE_ZOOM_SET";
            movieEvent->mRate = 0.5f; 
            return;
          case '4':
            movieEvent->mEventType = "MOVIE_ZOOM_SET";
            movieEvent->mRate = 4.0f; 
            return;
          case '$': // shift-4 = '$' = 1/4
            movieEvent->mEventType = "MOVIE_ZOOM_SET";
            movieEvent->mRate = 0.25f; 
            return;
          default:
            DEBUGMSG("unimplemented character '%c'", buffer[0]);
            break;
          }
        }
      }
    }
    break;
  case ButtonPress:
    movieEvent->mX = event.xbutton.x;
    movieEvent->mY = event.xbutton.y;
    if ( event.xbutton.button == Button1) // left mouse button
      movieEvent->mEventType = "MOVIE_MOUSE_PRESS_1";
    else if ( event.xbutton.button == Button2) // middle mouse button click
      movieEvent->mEventType = "MOVIE_MOUSE_PRESS_2";
    else if ( event.xbutton.button == Button3) // right mouse button
      movieEvent->mEventType = "MOVIE_MOUSE_PRESS_3";
    else if ( event.xbutton.button == Button4) // scroll wheel up
      movieEvent->mEventType = "MOVIE_ZOOM_UP";
    else if ( event.xbutton.button == Button5) // scroll wheel down
      movieEvent->mEventType = "MOVIE_ZOOM_DOWN";
    else 
      movieEvent->mEventType =   "MOVIE_NONE";
    return;
  case ButtonRelease:
    movieEvent->mX = event.xbutton.x;
    movieEvent->mY = event.xbutton.y;
    if ( event.xbutton.button == Button1)
      movieEvent->mEventType = "MOVIE_MOUSE_RELEASE_1";
    else if ( event.xbutton.button == Button2)
      movieEvent->mEventType = "MOVIE_MOUSE_RELEASE_2";
    else if ( event.xbutton.button == Button3)
      movieEvent->mEventType = "MOVIE_MOUSE_RELEASE_3";
    else 
      movieEvent->mEventType =   "MOVIE_NONE";
    return;
  case MotionNotify:
    /* filter/skip extra motion events */
    movieEvent->mX = event.xmotion.x;
    movieEvent->mY = event.xmotion.y;
    while (XPending(dpy)) {
      XPeekEvent(dpy, &event);
      if (event.type == MotionNotify) {
        XNextEvent(dpy, &event);
        movieEvent->mX = event.xmotion.x;
        movieEvent->mY = event.xmotion.y;
      }
      else
        break;
    }
    movieEvent->mEventType = "MOVIE_MOUSE_MOVE";
    return;
    
  default:
    DEBUGMSG("Got X event %d (%s)", event.type, EVENT_TYPE_STRING(event.type));
  }
  movieEvent->mEventType = "MOVIE_NONE";
  return;
}


// ==============================================================
/*
 * Close the X window associated with the canvas.
 */
void Renderer::Close(void)
{
  ECHO_FUNCTION(5);

  if (!mDisplay) return; 

  XDestroyWindow(mDisplay, mWindow);
  XFreeFont(mDisplay, mFontInfo);
  XFree(mVisualInfo);
  XFreeColormap(mDisplay, mColormap);
  XSync(mDisplay, 0);
  XCloseDisplay(mDisplay);

  return; 
}

// ==============================================================
// END  stuff from XWindow 
// ========================================================================
