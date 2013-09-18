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
Renderer * Renderer::CreateRenderer(ProgramOptions *opt,
                                    qint32 parentWindowID, 
                                    BlockbusterInterface *gui){
  QString name = opt->rendererName; 
  Renderer *renderer = NULL; 

  INFO("CreateRenderer creating renderer of type \"%s\"\n", name.toStdString().c_str()); 

  if (name == "gl" || name == "") 
    renderer = new glRenderer(opt, parentWindowID, gui); 
  if (name == "gl_stereo") 
    renderer = new glStereoRenderer(opt, parentWindowID, gui); 
  if (name == "gltexture") 
    renderer = new glTextureRenderer(opt, parentWindowID, gui); 
#ifdef USE_DMX
  if (name == "dmx") 
    renderer = new dmxRenderer(opt, parentWindowID, gui); 
#endif  
  if (name == "x11") 
    renderer = new x11Renderer(opt, parentWindowID, gui); 

  if (!renderer) {
    ERROR(QString("Badness:  cannot create renderer \"%1\"\n").
          arg(opt->rendererName)); 
    exit(1); 
  }
  // this has to be called after ChooseVisual() has been called
  renderer->FinishInit(opt); 
  return renderer;
  
}

// ======================================================================
Renderer::Renderer(ProgramOptions *opt, qint32 parentWindowID, 
                   BlockbusterInterface *gui, QString name):
  // from Canvas: 
  mHeight(0), mWidth(0), mScreenHeight(0), mScreenWidth(0), 
  mXPos(0), mYPos(0), mDepth(0), 
  mThreads(opt->readerThreads),
  mCacheSize(opt->mMaxCachedImages), 
  mBlockbusterInterface(gui), 
  // from XWindow: 
  mVisInfo(NULL), mScreenNumber(0), mWindow(0), mIsSubWindow(0), 
  mParentWindow(parentWindowID),
  mFontInfo(NULL), mFontHeight(0),  mShowCursor(true), 
  mOldWidth(-1), mOldHeight(-1), mOldX(-1), mOldY(-1), mXSync(false), 
  mName(name), mOptions(opt)
{ 

  // --------------------------------------
  // From XWindow:  
  mDisplay = XOpenDisplay(opt->displayName.toStdString().c_str());
  if (!mDisplay) {
    QString err("cannot open display '%1'"); 
    ERROR(err.arg(opt->displayName));
    return ;
  }
  
  /* If the user asked for synchronous behavior, provide it */
  if (mXSync) {
    (void) XSynchronize(mDisplay, True);
  }
  
  if (mParentWindow)
    mIsSubWindow = 1;

  opt->mRenderer = this; // ugh -- this needs to be fixed! 
  DEBUGMSG(QString("frameCacheSize is %1").arg(mCacheSize));    

  return; 
} 

// ======================================================================
void Renderer::FinishInit(ProgramOptions *opt) {
  ECHO_FUNCTION(5); 
  mVisInfo = ChooseVisual(); 
  FinishXWindowInit(opt); 
  FinishRendererInit(opt); 
  mCache.reset(new ImageCache(mOptions->readerThreads,
                              mOptions->mMaxCachedImages, mRequiredImageFormat));
  return;
}

// ======================================================================
void Renderer::SetFrameList(FrameListPtr frameList) {
  if (!mCache) {
    mCache.reset(new ImageCache(mThreads, mCacheSize, mRequiredImageFormat));
  }
  if (!mCache) {
    ERROR("could not recreate image cache when changing frame list");
    exit(1); 
  }
  
  /* Tell the cache to manage the new frame list.  This will
   * clear everything out of the cache that's already in there.
   */
  mCache->ManageFrameList(frameList); 
  mFrameList = frameList; 
}

// ======================================================================
void Renderer::Preload(uint32_t frameNumber,
                       const Rectangle *imageRegion, uint32_t levelOfDetail) {
  /* code stolen from cache.cpp CachePreload() 
     This is the appropriate code for all but DMX renderers. 
  */ 
  Rectangle lodROI;
  uint32_t localFrameNumber = 0;
  
  /* Adjust ROI for LOD! (very important) */
  lodROI.x = imageRegion->x >> levelOfDetail;
  lodROI.y = imageRegion->y >> levelOfDetail;
  lodROI.width = imageRegion->width >> levelOfDetail;
  lodROI.height = imageRegion->height >> levelOfDetail;
  
  
  if(mFrameList->stereo) {
    localFrameNumber = frameNumber * 2;
    mCache->PreloadImage( localFrameNumber++,
                          &lodROI, levelOfDetail);
    mCache->PreloadImage(localFrameNumber,
                         &lodROI, levelOfDetail);
  }
  else {
    localFrameNumber = frameNumber;
    mCache->PreloadImage(localFrameNumber,
                         &lodROI, levelOfDetail);
  }   
  return; 
}

//=====================================================================
/* 
   Preload a length of frames
   startFrame -- 
*/ 
void Renderer::Preload(uint32_t frameNumber, uint32_t preloadFrames, 
                       int playDirection, 
                       uint32_t minFrame, uint32_t maxFrame,
                       const Rectangle *imageRegion, 
                       uint32_t levelOfDetail) {
  
  uint32_t preloadmax = maxFrame-minFrame, 
    frame=frameNumber, preloaded=0;
  if (preloadmax > preloadFrames) {
    preloadmax = preloadFrames;
  }
  while (preloaded++ < preloadmax) {
    frame += playDirection; 
    if (frame > maxFrame) frame = minFrame; 
    if (frame < minFrame) frame = maxFrame; 
    DEBUGMSG("Preload frame %d", frame); 
    this->Preload(frame, imageRegion, levelOfDetail);
  }
  return; 
}


//=========================================================================
// BEGIN stuff from Canvas: 
// ==============================================================
//============================================================
/* Saves the specified frame to a file as
 * a compilable Image file. 
 *
 */

void Renderer::WriteImageToFile(int frameNumber)
{
  ImagePtr image;
  Rectangle region;
  FILE *f;
  register uint32_t i;
  register unsigned char *imageData;
  uint32_t localFrameNumber = 0;

  /* explicitly use localFrameNumber as a reminder that we may be dealing with stereo movies */
  if(mFrameList->stereo) {
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

  image = GetImage(localFrameNumber, &region, 0);

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
  /* Added to support stereo files */
  /* Assumes we have a valid FrameListPtr */
  int localFrameNumber = 0;

  if(mFrameList->stereo) {
	localFrameNumber = frameNumber * 2;
	
  }
  else {
	localFrameNumber = frameNumber;
  }
  
  return mFrameList->getFrame(localFrameNumber);
}



//============================================================
void Renderer::ReportFrameListChange(FrameListPtr iframeList) {
  mFrameList = iframeList; 
  DEBUGMSG("Renderer::ReportFrameListChange"); 
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

//============================================================
void Renderer::ReportFrameChange(int frameNumber) {
  DEBUGMSG("Renderer::ReportFrameChange %d", frameNumber); 
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameNumber(frameNumber+1); 
  }
  return; 
}

//============================================================
void Renderer::ReportDetailRangeChange(int min, int max) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setLODRange(min,max); 
    mBlockbusterInterface->setLOD(1); 
  }
  return; 
}

//============================================================
void Renderer::ReportDetailChange(int levelOfDetail) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setLOD(levelOfDetail); 
  }
  return; 
}

//============================================================
void Renderer::ReportRateRangeChange(float min, float max) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameRateRange(min,max); 
  }
  return; 
}

//============================================================
void Renderer::ReportLoopBehaviorChange(int behavior) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setLoopBehavior(behavior); 
  }
  return; 
}

//============================================================
void Renderer::ReportPingPongBehaviorChange(int behavior) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setPingPongBehavior(behavior); 
  }
  return; 
}

//============================================================
void Renderer::ReportRateChange(float rate) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setFrameRate(rate); 
  }
  return; 
}

//============================================================
void Renderer::ReportZoomChange(float zoom) {
  if (mBlockbusterInterface) {
    mBlockbusterInterface->setZoom(zoom); 
  }
  return; 
}

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
void Renderer::reportWindowMoved(int xpos, int ypos){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportWindowMoved(xpos, ypos); 
  }
  return ;
}

//============================================
void Renderer::reportWindowResize(int x, int y){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportWindowResize(x,y); 
  }
  return ;
} 

//============================================
void Renderer::reportMovieMoved(int xpos, int ypos){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieMoved(xpos, ypos); 
  }
  return ;
} 
 
//============================================
void Renderer::reportMovieFrameSize(int x, int y){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieFrameSize(x,y);
  }
  return ;
} 

//============================================
void Renderer::reportMovieDisplayedSize(int x, int y){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportMovieDisplayedSize(x,y); 
  }
  return ;
} 

//============================================
void Renderer::reportActualFPS(double rate){
  if (mBlockbusterInterface) {
    mBlockbusterInterface->reportActualFPS(rate); 
  }
  return ;
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

// ==============================================================
// END  stuff from Canvas 
// ========================================================================

// ========================================================================
// BEGIN stuff from XWindow (all public): 
// ==============================================================
// ==============================================================
void Renderer::FinishXWindowInit(ProgramOptions *options) {
  ECHO_FUNCTION(5); 
  const Rectangle *geometry = &options->geometry;
  int decorations = options->decorations;
  QString suggestedName = options->suggestedTitle;
  Screen *screen;
  int x, y, width, height;
  int required_x_margin, required_y_margin;
  /* Get the screen and do some sanity checks */
  mScreenNumber = DefaultScreen(mDisplay);
  screen = ScreenOfDisplay(mDisplay, mScreenNumber);
  
  /* if geometry is don't care and decorations flag is off -- then set window to max screen extents */
  mScreenWidth = WidthOfScreen(screen);
  if (geometry->width != DONT_CARE) 
    width = geometry->width;
  else {
    if(decorations) 
      width = DEFAULT_WIDTH;
    else 
      width =  mScreenWidth;
  }
  
  mScreenHeight = HeightOfScreen(screen);
  if (geometry->height != DONT_CARE) 
    height = geometry->height;
  else {
    if(decorations)
      height = DEFAULT_HEIGHT;
    else
      height =  mScreenHeight; 
  }
  
  
  
  /* if we've turned off the window border (decoration) with the -D flag then set rquired margins to zero
     otherwise set them to the constants defined in movie.h (SCREEN_X_MARGIN ... ) */
  if (decorations) {
    required_x_margin = SCREEN_X_MARGIN;
    required_y_margin = SCREEN_Y_MARGIN;
  }
  else {
    required_x_margin = 0;
    required_y_margin = 0;
  }
  
  
  if (width > WidthOfScreen(screen) - required_x_margin) {
#if 0
    WARNING("requested window width %d greater than screen width %d",
            width, WidthOfScreen(screen));
#endif
    width = WidthOfScreen(screen) - required_x_margin;
  }
  if (height > HeightOfScreen(screen) - required_y_margin) {
#if 0
    WARNING("requested window height %d greater than screen height %d",
            height, HeightOfScreen(screen));
#endif
    height = HeightOfScreen(screen) - required_y_margin;
  }
  
  
  if (geometry->x == CENTER)
    x = (WidthOfScreen(screen) - width) / 2;
  else if (geometry->x != DONT_CARE)
    x = geometry->x;
  else
    x = 0;
  
  if (geometry->y == CENTER)
    y = (HeightOfScreen(screen) - height) / 2;
  else if (geometry->y != DONT_CARE)
    y = geometry->y;
  else
    y = 0;
 

  XSetWindowAttributes windowAttrs;
  unsigned long windowAttrsMask;
  XSizeHints sizeHints;
  const int winBorder = 0;
  //mVisInfo = this->ChooseVisual( ); // virtual function 
  if (mVisInfo == NULL) {
    XCloseDisplay(mDisplay);
    ERROR("Could not get mVisInfo in XWindow constructor.\n"); 
  }

  /* Set up desired window attributes */
  mColormap = XCreateColormap(mDisplay, RootWindow(mDisplay, mScreenNumber),
                              mVisInfo->visual, AllocNone);
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
  mWindow = XCreateWindow(mDisplay, mParentWindow,
                          x, y, width, height,
                          winBorder, mVisInfo->depth, InputOutput,
                          mVisInfo->visual, windowAttrsMask, &windowAttrs);
  

  DEBUGMSG("created window 0x%x", mWindow);
  
  /* Pass some information along to the window manager to keep it happy */
  sizeHints.flags = USSize;
  sizeHints.width = width;
  sizeHints.height = height;
  if (geometry->x != DONT_CARE && geometry->y != DONT_CARE) {
    sizeHints.x = geometry->x;
    sizeHints.y = geometry->y;
    sizeHints.flags |= USPosition;
  }
  
  
  XSetNormalHints(mDisplay, mWindow, &sizeHints);
  
  
  SetTitle(suggestedName); 
  XSetStandardProperties(mDisplay, mWindow, 
                         suggestedName.toAscii(), suggestedName.toAscii(), 
                         None, (char **)NULL, 0, &sizeHints);
  
  
  if (!decorations) remove_mwm_border();
  
  /* Bring it up; then wait for it to actually get here. */
  XMapWindow(mDisplay, mWindow);
  XSync(mDisplay, 0);
  /*
    XIfEvent(mDisplay, &event, WaitForWindowOpen, (XPointer) sWindowInfo);
  */
  
  /* The font.  All renderers that attach to this user interface will use
   * this font for rendering status information directly into the rendering
   * window.
   */
  mFontInfo = XLoadQueryFont(mDisplay, 
                             options->fontName.toAscii());
  if (!mFontInfo) {
    QString warning("Couldn't load font %s, trying %s");
    WARNING(warning.arg(options->fontName).arg(DEFAULT_X_FONT));
    
    mFontInfo = XLoadQueryFont(mDisplay,
                               DEFAULT_X_FONT);
    if (!mFontInfo) {
      ERROR("Couldn't load DEFAULT FONT %s", DEFAULT_X_FONT);
      XFree(mVisInfo);
      XFreeColormap(mDisplay, mColormap);
      XCloseDisplay(mDisplay);
      return ;
    }
  }
  mFontHeight = mFontInfo->ascent + mFontInfo->descent;


  XWindowAttributes win_attributes; 
  XGetWindowAttributes(mDisplay, mWindow, &win_attributes); 
  /*  XTranslateCoordinates (mDisplay, mWindow, win_attributes.root, 
      -win_attributes.border_width,
      -win_attributes.border_width,
      &x, &y, &junkwin); */

  printf("New X,Y, border width is %d, %d, %d\n", x,y, win_attributes.border_width); 
  XGetWindowAttributes(mDisplay, mParentWindow, &win_attributes); 
  printf("ParentWindow: X,Y =  %d, %d\n", win_attributes.x, win_attributes.y);
  //SetCanvasAttributes(mWindow); 
  mWidth = width;
  mHeight = height;
  mXPos = x; 
  mYPos = y; 
  mDepth = mVisInfo->depth;
  return; 
}// END CONSTRUCTOR for XWindow

/* SetCanvasAttributes(Window window) {
   Window junkwin; 
   XWindowAttributes win_attributes; 
   XGetWindowAttributes(mDisplay, mWindow, &win_attributes); 
   XTranslateCoordinates (mDisplay, mWindow, win_attributes.root, 
   -win_attributes.border_width,
   -win_attributes.border_width,
   &x, &y, &junkwin); 

   printf("New X,Y, border width is %d, %d, %d\n", x,y, win_attributes.border_width); 
   // Prepare our Canvas structure that we can return to the caller 
   mWidth = width;
   mHeight = height;
   mXPos = x; 
   mYPos = y; 
   mDepth = mVisInfo->depth;
  
 
  
   return ;
   } 
*/
 
// ==============================================================
/*
 * Helper to remove window decorations
 */
void Renderer::remove_mwm_border(void )
{
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
  motif_hints.decorations = 0;
  
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
  return; 
}


/* Possible swap actions: 
   {"undefined", XdbeUndefined, "back buffer becomes undefined on swap"},
   -    {"background", XdbeBackground, "back buffer is cleared to window background on swap"},
   -    {"untouched", XdbeUntouched, "back buffer is contents of front buffer on swap"},
   -    {"copied", XdbeCopied, "back buffer is held constant on swap"},
*/ 

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
    if(blank == None) fprintf(stderr, "error: out of memory.\n");
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
  XStoreName(mDisplay, mWindow, title.toAscii().data()); 
  return; 
}

// ==============================================================
void Renderer::Resize(int newWidth, int newHeight, int cameFromX){
  ECHO_FUNCTION(5);
  dbprintf(5, "Renderer::Resize(%d, %d, %d)\n", newWidth, newHeight, cameFromX); 
  if (cameFromX) {
    mHeight = newHeight; 
    mWidth = newWidth; 
    return; 
  }
  XWindowChanges values;
  unsigned int mask;
  
  if (mWidth == newWidth && mHeight == newHeight)
    return;
  
  /* X windows must be at least 1x1 */
  if (newWidth == 0)
    newWidth = 1;
  if (newHeight == 0)
    newHeight = 1;
  
  
  values.width = newWidth;
  values.height = newHeight;
  mask = CWWidth | CWHeight;
  XConfigureWindow(mDisplay, mWindow, mask, &values);
  /* Force sync, in case we get no events (dmx) */
  XSync(mDisplay, 0);
  
  mWidth = newWidth;
  mHeight = newHeight;
  
  return; 
}


// ===============================================================
/*
 * Move the X window to given position 
 */
void Renderer::Move(int newX, int newY, int cameFromX) {
  ECHO_FUNCTION(5);
  //cerr << "MoveXWindow"<<endl;
  if (cameFromX) {
    mXPos = newX; 
    mYPos = newY; 
    return; 
  }
  XMoveWindow(mDisplay, mWindow, newX, newY);
  mXPos = newX;  
  mYPos = newY; 
  /* Force sync, in case we get no events (dmx) */
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
    fprintf(stderr, "Warning:  fakeMouseClick() called with no active mDisplay.\n"); 
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
    fprintf(stderr, "Warning: Error with XSendEvent in fakeMouseClick()\n");
    }
  */
  if(XSendEvent(mDisplay, mWindow, True, 0xfff, &event) == 0)  {
    fprintf(stderr, "Warning: Error with XSendEvent in fakeMouseClick()\n");
  }

  XFlush(mDisplay);
  
  //usleep(100*000); // ? Why is this included? 
  
  event.type = ButtonRelease;
  event.xbutton.state = 0x100;
  
  /*
    if(XSendEvent(mDisplay, PointerWindow, True, 0xfff, &event) == 0) {
    fprintf(stderr, "Warning: Error with XSendEvent in fakeMouseClick()\n");
    }
  */
  if(XSendEvent(mDisplay, mWindow, True, 0xfff, &event) == 0) {
    fprintf(stderr, "Warning: Error with XSendEvent in fakeMouseClick()\n");
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
 * the idle event (MOVIE_NONE) if no event is present and
 * we aren't supposed to block waiting for input.
 */
void Renderer::GetXEvent(int block, MovieEvent *movieEvent)
{
  bool resize=false, move=false; 
  XEvent event;
  Display *dpy = mDisplay;
  if (!block && !XPending(dpy)) {
    movieEvent->mEventType = MOVIE_NONE;
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
    movieEvent->mEventType = MOVIE_EXPOSE;
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
      movieEvent->mEventType = MOVIE_RESIZE;
    }
    
    if (mOldX != event.xconfigure.x || mOldY != event.xconfigure.y) {
      move = true; 
      mOldX = event.xconfigure.x;
      mOldY = event.xconfigure.y;
      movieEvent->mEventType = MOVIE_MOVE;
    }
    
    
    /* do not process these events here, as they are spurious
       if (resize && move)  movieEvent->mEventType = MOVIE_MOVE_RESIZE;
       else*/
    if (resize)  movieEvent->mEventType = MOVIE_RESIZE;
    else if (move)  movieEvent->mEventType = MOVIE_MOVE;
    else movieEvent->mEventType = MOVIE_NONE;
    
    return;
  case KeyPress:
    {
      int code = XLookupKeysym(&event.xkey, 0);
      if (code == XK_Left && event.xkey.state & ControlMask) {
        movieEvent->mEventType = MOVIE_SECTION_BACKWARD;
        return;
      }
      if (code == XK_Left && event.xkey.state & ShiftMask) {
        movieEvent->mEventType = MOVIE_SKIP_BACKWARD;
        return;
      }
      else if (code == XK_Left) {
        movieEvent->mEventType = MOVIE_STEP_BACKWARD;
        return;
      }
      else if (code == XK_Right && event.xkey.state & ControlMask) {
        movieEvent->mEventType = MOVIE_SECTION_FORWARD;
        return;
      }
      else if (code == XK_Right && event.xkey.state & ShiftMask) {
        movieEvent->mEventType = MOVIE_SKIP_FORWARD;
        return;
      }
      else if (code == XK_Right) {
        movieEvent->mEventType = MOVIE_STEP_FORWARD;
        return;
      }
      else if (code == XK_Home) {
        movieEvent->mEventType = MOVIE_GOTO_START;
        return;
      }
      else if (code == XK_Escape) {
        movieEvent->mEventType = MOVIE_QUIT;
        return;
      }  
      else if (code == XK_End) {
        movieEvent->mEventType = MOVIE_GOTO_END;
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
            movieEvent->mEventType = MOVIE_CENTER;
            return;
          case 'p':
            movieEvent->mEventType = MOVIE_PLAY_FORWARD;
            return;
          case 'q':
            movieEvent->mEventType = MOVIE_QUIT;
            return;
          case 'r':
            movieEvent->mEventType = MOVIE_PLAY_BACKWARD;
            return;
          case 'z':
            movieEvent->mEventType = MOVIE_ZOOM_UP;
            return;
          case 'Z':
            movieEvent->mEventType = MOVIE_ZOOM_DOWN;
            return;
          case ' ':
            movieEvent->mEventType = MOVIE_PAUSE;
            return;
          case 'f':
            movieEvent->mEventType = MOVIE_ZOOM_FIT;
            return;
          case '1':
            movieEvent->mEventType = MOVIE_ZOOM_ONE;
            return;
          case '+':
            movieEvent->mEventType = MOVIE_INCREASE_RATE;
            return;
          case '-':
            movieEvent->mEventType = MOVIE_DECREASE_RATE;
            return;
          case 'l':
            movieEvent->mEventType = MOVIE_INCREASE_LOD;
            return;
          case 'L':
            movieEvent->mEventType = MOVIE_DECREASE_LOD;
            return;
          case 'i':
            movieEvent->mEventType = MOVIE_TOGGLE_INTERFACE;
            return;
          case '?':
          case 'h':
            PrintKeyboardControls();
            break;
          case 'm':
            ToggleCursor(); 
            break; 
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
    movieEvent->mEventType = 
      event.xbutton.button == Button1 ? MOVIE_MOUSE_PRESS_1:
      event.xbutton.button == Button2 ? MOVIE_MOUSE_PRESS_2:
      event.xbutton.button == Button3 ? MOVIE_MOUSE_PRESS_3:
      MOVIE_NONE;
    return;
  case ButtonRelease:
    movieEvent->mX = event.xbutton.x;
    movieEvent->mY = event.xbutton.y;
    movieEvent->mEventType = 
      event.xbutton.button == Button1 ? MOVIE_MOUSE_RELEASE_1:
      event.xbutton.button == Button2 ? MOVIE_MOUSE_RELEASE_2:
      event.xbutton.button == Button3 ? MOVIE_MOUSE_RELEASE_3:
      MOVIE_NONE;
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
    movieEvent->mEventType = MOVIE_MOUSE_MOVE;
    return;
    
  default:
    DEBUGMSG("Got X event %d (%s)", event.type, EVENT_TYPE_STRING(event.type));
  }
  movieEvent->mEventType = MOVIE_NONE;
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
  XFree(mVisInfo);
  XFreeColormap(mDisplay, mColormap);
  XSync(mDisplay, 0);
  XCloseDisplay(mDisplay);

  return; 
}

// ==============================================================
// END  stuff from XWindow 
// ========================================================================
