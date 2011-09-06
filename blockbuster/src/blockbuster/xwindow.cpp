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



#include "canvas.h"
#include "frames.h"
#include "errmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "xwindow.h"
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600


/* This function is called to initialize an already-allocated Canvas.
 * The Glue information is already copied into place.
 */
XWindow::XWindow(ProgramOptions *options, Canvas *theCanvas, Window parentWin):
  mOptions(options), mCanvas(theCanvas), display(NULL), 
  visInfo(NULL), screenNumber(0), window(0), isSubWindow(0), 
  fontInfo(NULL), fontHeight(0),  mShowCursor(true), 
  mOldWidth(-1), mOldHeight(-1), mOldX(-1), mOldY(-1), mXSync(false) {
  ECHO_FUNCTION(5);
  
      
  //display = XOpenDisplay(DisplayString(QX11Info::display())); 
  //if (!display) {
  //  dbprintf(0, QString("Opening display %1\n").arg(options->displayName)); 
  display = XOpenDisplay(options->displayName.toStdString().c_str());
  //}
  if (!display) {
    QString err("cannot open display '%1'"); 
    ERROR(err.arg(options->displayName));
    return ;
  }
  
  /* If the user asked for synchronous behavior, provide it */
  if (mXSync) {
    (void) XSynchronize(display, True);
  }
  
  if (parentWin)
    isSubWindow = 1;
  
 
   return ;
}
  
  
void XWindow::FinishXWindowInit(ProgramOptions *options, Canvas *, Window parentWin) {
  ECHO_FUNCTION(5); 
  const Rectangle *geometry = &options->geometry;
  int decorations = options->decorations;
  QString suggestedName = options->suggestedTitle;
  Screen *screen;
  int x, y, width, height;
  int required_x_margin, required_y_margin;
  /* Get the screen and do some sanity checks */
  screenNumber = DefaultScreen(display);
  screen = ScreenOfDisplay(display, screenNumber);
  
  /* if geometry is don't care and decorations flag is off -- then set window to max screen extents */
  mCanvas->screenWidth = WidthOfScreen(screen);
  if (geometry->width != DONT_CARE) 
    width = geometry->width;
  else {
    if(decorations) 
      width = DEFAULT_WIDTH;
    else 
      width =  mCanvas->screenWidth;
  }
  
  mCanvas->screenHeight = HeightOfScreen(screen);
  if (geometry->height != DONT_CARE) 
    height = geometry->height;
  else {
    if(decorations)
      height = DEFAULT_HEIGHT;
    else
      height =  mCanvas->screenHeight; 
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
  //visInfo = this->ChooseVisual( ); // virtual function 
  if (visInfo == NULL) {
    XCloseDisplay(display);
    ERROR("Could not get visInfo in XWindow constructor.\n"); 
  }

  /* Set up desired window attributes */
  colormap = XCreateColormap(display, RootWindow(display, screenNumber),
                             visInfo->visual, AllocNone);
  windowAttrsMask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
  windowAttrs.background_pixel = 0x0;
  windowAttrs.border_pixel = 0x0;
  windowAttrs.colormap = colormap;
  windowAttrs.event_mask = StructureNotifyMask | ExposureMask;
  if (parentWin) {
    /* propogate mouse/keyboard events to parent */
  }
  else {
    windowAttrs.event_mask |= (KeyPressMask | ButtonPressMask |
                               ButtonReleaseMask | ButtonMotionMask);
  }
  
  if (!parentWin)
    parentWin = RootWindow(display, screenNumber);
  
  printf ("XCreateWindow: X,Y = %d, %d\n", x, y); 
  window = XCreateWindow(display, parentWin,
                         x, y, width, height,
                         winBorder, visInfo->depth, InputOutput,
                         visInfo->visual, windowAttrsMask, &windowAttrs);
  

  DEBUGMSG("created window 0x%x", window);
  
  /* Pass some information along to the window manager to keep it happy */
  sizeHints.flags = USSize;
  sizeHints.width = width;
  sizeHints.height = height;
  if (geometry->x != DONT_CARE && geometry->y != DONT_CARE) {
    sizeHints.x = geometry->x;
    sizeHints.y = geometry->y;
    sizeHints.flags |= USPosition;
  }
  
  
  XSetNormalHints(display, window, &sizeHints);
  
  
  SetTitle(suggestedName); 
  XSetStandardProperties(display, window, 
                         suggestedName.toAscii(), suggestedName.toAscii(), 
                         None, (char **)NULL, 0, &sizeHints);
  
  
  if (!decorations) remove_mwm_border();
  
  /* Bring it up; then wait for it to actually get here. */
  XMapWindow(display, window);
  XSync(display, 0);
  /*
    XIfEvent(display, &event, WaitForWindowOpen, (XPointer) sWindowInfo);
  */
  
  /* The font.  All renderers that attach to this user interface will use
   * this font for rendering status information directly into the rendering
   * window.
   */
  fontInfo = XLoadQueryFont(display, 
                                         options->fontName.toAscii());
  if (!fontInfo) {
    QString warning("Couldn't load font %s, trying %s");
    WARNING(warning.arg(options->fontName).arg(DEFAULT_X_FONT));
    
    fontInfo = XLoadQueryFont(display,
                                           DEFAULT_X_FONT);
    if (!fontInfo) {
      ERROR("Couldn't load DEFAULT FONT %s", DEFAULT_X_FONT);
      XFree(visInfo);
      XFreeColormap(display, colormap);
      XCloseDisplay(display);
      return ;
    }
  }
  fontHeight = fontInfo->ascent + fontInfo->descent;


  XWindowAttributes win_attributes; 
  XGetWindowAttributes(display, window, &win_attributes); 
  /*  XTranslateCoordinates (display, window, win_attributes.root, 
                         -win_attributes.border_width,
                         -win_attributes.border_width,
                         &x, &y, &junkwin); */

  printf("New X,Y, border width is %d, %d, %d\n", x,y, win_attributes.border_width); 
  XGetWindowAttributes(display, parentWin, &win_attributes); 
  printf("ParentWindow: X,Y =  %d, %d\n", win_attributes.x, win_attributes.y);
  //SetCanvasAttributes(window); 
  mCanvas->width = width;
  mCanvas->height = height;
  mCanvas->XPos = x; 
  mCanvas->YPos = y; 
  mCanvas->depth = visInfo->depth;
  return; 
}// END CONSTRUCTOR for XWindow

/* SetCanvasAttributes(Window window) {
  Window junkwin; 
  XWindowAttributes win_attributes; 
  XGetWindowAttributes(display, window, &win_attributes); 
  XTranslateCoordinates (display, window, win_attributes.root, 
                         -win_attributes.border_width,
                         -win_attributes.border_width,
                         &x, &y, &junkwin); 

  printf("New X,Y, border width is %d, %d, %d\n", x,y, win_attributes.border_width); 
  // Prepare our Canvas structure that we can return to the caller 
  mCanvas->width = width;
  mCanvas->height = height;
  mCanvas->XPos = x; 
  mCanvas->YPos = y; 
  mCanvas->depth = visInfo->depth;
  
 
  
  return ;
} 
*/
 
/*
 * Helper to remove window decorations
 */
void XWindow::remove_mwm_border(void )
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
  prop = XInternAtom(display, "_MOTIF_WM_HINTS", True );
  if (!prop) {
    /* something went wrong! */
    return;
  }
  
  /* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
  proptype = prop;
  
  XChangeProperty( display, window,  /* display, window */
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
void XWindow::ShowCursor(bool show) {
  DEBUGMSG("Show cursor: %d\n", show); 
  //cerr << "Show cursor: "<< show; 
  
  if (!show) {
    /* make a blank cursor */
    Pixmap blank;
    XColor dummy;
    char data[1] = {0};
    Cursor cursor;
    
    blank = XCreateBitmapFromData (display, window, data, 1, 1); 
    if(blank == None) fprintf(stderr, "error: out of memory.\n");
    cursor = XCreatePixmapCursor(display, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap (display, blank);
    
    
    //this makes you the cursor. then set it using this function
    XDefineCursor(display,  window,  cursor);
  } else {
    if (!mShowCursor) {
      XUndefineCursor(display, window);
    }
  }
  mShowCursor = show; 
  return; 
}

//======================================================   
void XWindow::ToggleCursor(void) {
  ECHO_FUNCTION(5);
  ShowCursor(!mShowCursor); 
  return; 
}

//======================================================   
void XWindow::SetTitle(QString title) {
  ECHO_FUNCTION(5);
  XStoreName(display, window, title.toAscii().data()); 
  return; 
}

void XWindow::Resize(int newWidth, int newHeight, int cameFromX){
  ECHO_FUNCTION(5);
  if (cameFromX) {
    mCanvas->height = newHeight; 
    mCanvas->width = newWidth; 
    return; 
  }
  XWindowChanges values;
  unsigned int mask;
  
  if (mCanvas->width == newWidth && mCanvas->height == newHeight)
    return;
  
  /* X windows must be at least 1x1 */
  if (newWidth == 0)
    newWidth = 1;
  if (newHeight == 0)
    newHeight = 1;
  
  
  values.width = newWidth;
  values.height = newHeight;
  mask = CWWidth | CWHeight;
  XConfigureWindow(display, window, mask, &values);
  /* Force sync, in case we get no events (dmx) */
  XSync(display, 0);
  
  mCanvas->width = newWidth;
  mCanvas->height = newHeight;
  
  return; 
}


// ===============================================================
/*
 * Move the X window to given position 
 */
void XWindow::Move(int newX, int newY, int cameFromX) {
  ECHO_FUNCTION(5);
  //cerr << "MoveXWindow"<<endl;
  if (cameFromX) {
    mCanvas->XPos = newX; 
    mCanvas->YPos = newY; 
    return; 
  }
  XMoveWindow(display, window, newX, newY);
  mCanvas->XPos = newX;  
  mCanvas->YPos = newY; 
  /* Force sync, in case we get no events (dmx) */
  XSync(display, 0);
}

// ===============================================================
/*!
  Added to keep the screensaver from kicking in during a movie. 
  Moves the pointer to a certain spot and clicks there.  
  Code stolen from https://gist.github.com/726474
  Written by Pioz, whoever that is.  
  Modified by Rich Cook.  Please don't sue me.  
*/ 
void XWindow::fakeMouseClick(void)
{
  XEvent event;
  
  if(display == NULL)  {
    fprintf(stderr, "Warning:  fakeMouseClick() called with no active display.\n"); 
    return; 
  }
  
  memset(&event, 0x00, sizeof(event));
  
  event.type = ButtonPress;
  event.xbutton.button = Button1;
  event.xbutton.same_screen = True;
  
  XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
  
  event.xbutton.subwindow = event.xbutton.window;
  
  while(event.xbutton.subwindow)	{
    event.xbutton.window = event.xbutton.subwindow;
    
    XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
  }
  
  if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0)  {
    fprintf(stderr, "Warning: Error with XSendEvent in fakeMouseClick()\n");
  }
  
  XFlush(display);
  
  //usleep(100*000); // ? Why is this included? 
  
  event.type = ButtonRelease;
  event.xbutton.state = 0x100;
  
  if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) {
    fprintf(stderr, "Warning: Error with XSendEvent in fakeMouseClick()\n");
  }
  return; 
}
/*
 * Helper macro used by the renderers
 */
#define EVENT_TYPE_STRING(x) (                  \
                              x==MapNotify?"MapNotify": \
                              x==UnmapNotify?"UnmapNotify": \
                              x==ReparentNotify?"ReparentNotify":   \
                              "unknown")


/*
 * Get the next event from the given Canvas's X window, or
 * the idle event (MOVIE_NONE) if no event is present and
 * we aren't supposed to block waiting for input.
 */
void XWindow::GetXEvent(int block, MovieEvent *movieEvent)
{
  bool resize=false, move=false; 
  XEvent event;
  Display *dpy = display;
  if (!block && !XPending(dpy)) {
    movieEvent->eventType = MOVIE_NONE;
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
    movieEvent->eventType = MOVIE_EXPOSE;
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
    movieEvent->width = event.xconfigure.width;
    movieEvent->height = event.xconfigure.height;
    movieEvent->x = event.xconfigure.x;
    movieEvent->y = event.xconfigure.y;
    movieEvent->number = 1; // means "I got this from X, so don't call XMoveResize again" -- to prevent certain loops from happening, especially on the Mac where the window will keep marching down the screen.  Other window managers might have similar pathologies, so I did not implement a Mac-specific hack. 
    
    // Check if the size really changed.  If it did, send the event.
    // Suppress it otherwise.
    if (mOldHeight != event.xconfigure.height || 
        mOldWidth != event.xconfigure.width) {
      resize = true; 
      mOldHeight = event.xconfigure.height;
      mOldWidth = event.xconfigure.width;
      movieEvent->eventType = MOVIE_RESIZE;
    }
    
    if (mOldX != event.xconfigure.x || mOldY != event.xconfigure.y) {
      move = true; 
      mOldX = event.xconfigure.x;
      mOldY = event.xconfigure.y;
      movieEvent->eventType = MOVIE_MOVE;
    }
    
    
    /* do not process these events here, as they are spurious
       if (resize && move)  movieEvent->eventType = MOVIE_MOVE_RESIZE;
       else*/
    if (resize)  movieEvent->eventType = MOVIE_RESIZE;
    else if (move)  movieEvent->eventType = MOVIE_MOVE;
    else movieEvent->eventType = MOVIE_NONE;
    
    return;
  case KeyPress:
    {
      int code = XLookupKeysym(&event.xkey, 0);
      if (code == XK_Left && event.xkey.state & ControlMask) {
        movieEvent->eventType = MOVIE_SECTION_BACKWARD;
        return;
      }
      if (code == XK_Left && event.xkey.state & ShiftMask) {
        movieEvent->eventType = MOVIE_SKIP_BACKWARD;
        return;
      }
      else if (code == XK_Left) {
        movieEvent->eventType = MOVIE_STEP_BACKWARD;
        return;
      }
      else if (code == XK_Right && event.xkey.state & ControlMask) {
        movieEvent->eventType = MOVIE_SECTION_FORWARD;
        return;
      }
      else if (code == XK_Right && event.xkey.state & ShiftMask) {
        movieEvent->eventType = MOVIE_SKIP_FORWARD;
        return;
      }
      else if (code == XK_Right) {
        movieEvent->eventType = MOVIE_STEP_FORWARD;
        return;
      }
      else if (code == XK_Home) {
        movieEvent->eventType = MOVIE_GOTO_START;
        return;
      }
      else if (code == XK_Escape) {
        movieEvent->eventType = MOVIE_QUIT;
        return;
      }  
      else if (code == XK_End) {
        movieEvent->eventType = MOVIE_GOTO_END;
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
            movieEvent->eventType = MOVIE_CENTER;
            return;
          case 'p':
            movieEvent->eventType = MOVIE_PLAY_FORWARD;
            return;
          case 'q':
            movieEvent->eventType = MOVIE_QUIT;
            return;
          case 'r':
            movieEvent->eventType = MOVIE_PLAY_BACKWARD;
            return;
          case 'z':
            movieEvent->eventType = MOVIE_ZOOM_UP;
            return;
          case 'Z':
            movieEvent->eventType = MOVIE_ZOOM_DOWN;
            return;
          case ' ':
            movieEvent->eventType = MOVIE_PAUSE;
            return;
          case 'f':
            movieEvent->eventType = MOVIE_ZOOM_FIT;
            return;
          case '1':
            movieEvent->eventType = MOVIE_ZOOM_ONE;
            return;
          case '+':
            movieEvent->eventType = MOVIE_INCREASE_RATE;
            return;
          case '-':
            movieEvent->eventType = MOVIE_DECREASE_RATE;
            return;
          case 'l':
            movieEvent->eventType = MOVIE_INCREASE_LOD;
            return;
          case 'L':
            movieEvent->eventType = MOVIE_DECREASE_LOD;
            return;
          case 'i':
            movieEvent->eventType = MOVIE_TOGGLE_INTERFACE;
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
    movieEvent->x = event.xbutton.x;
    movieEvent->y = event.xbutton.y;
    movieEvent->eventType = 
      event.xbutton.button == Button1 ? MOVIE_MOUSE_PRESS_1:
      event.xbutton.button == Button2 ? MOVIE_MOUSE_PRESS_2:
      event.xbutton.button == Button3 ? MOVIE_MOUSE_PRESS_3:
      MOVIE_NONE;
    return;
  case ButtonRelease:
    movieEvent->x = event.xbutton.x;
    movieEvent->y = event.xbutton.y;
    movieEvent->eventType = 
      event.xbutton.button == Button1 ? MOVIE_MOUSE_RELEASE_1:
      event.xbutton.button == Button2 ? MOVIE_MOUSE_RELEASE_2:
      event.xbutton.button == Button3 ? MOVIE_MOUSE_RELEASE_3:
      MOVIE_NONE;
    return;
  case MotionNotify:
    /* filter/skip extra motion events */
    movieEvent->x = event.xmotion.x;
    movieEvent->y = event.xmotion.y;
    while (XPending(dpy)) {
      XPeekEvent(dpy, &event);
      if (event.type == MotionNotify) {
        XNextEvent(dpy, &event);
        movieEvent->x = event.xmotion.x;
        movieEvent->y = event.xmotion.y;
      }
      else
        break;
    }
    movieEvent->eventType = MOVIE_MOUSE_MOVE;
    return;
    
  default:
    DEBUGMSG("Got X event %d (%s)", event.type, EVENT_TYPE_STRING(event.type));
  }
  movieEvent->eventType = MOVIE_NONE;
  return;
}


/*
 * Close the X window associated with the canvas.
 */
void XWindow::Close(void)
{
  ECHO_FUNCTION(5);
  /* Give the Glue routines a chance to free themselves */
  if (mCanvas->mRenderer != NULL) {
    /* Give the Glue routines a chance to free themselves */
    XDestroyWindow(display, window);
    XFreeFont(display, fontInfo);
    XFree(visInfo);
    XFreeColormap(display, colormap);
    XSync(display, 0);
    XCloseDisplay(display);
    
  }

  return; 
}

//======================================================   
// END XWindow struct
//======================================================   
