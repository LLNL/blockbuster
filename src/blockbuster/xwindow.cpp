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

/*
 * This module encapsulates the X11 UserInterface, and includes all the
 * "glue" required by the renderers that the X11 UserInterface supports.
 */


#include "canvas.h"
#include "frames.h"
#include "errmsg.h"
#include "dmxglue.h"
#include "Renderers.h"
#include "glRenderer.h"
#include "x11Renderer.h"
#include "gltexture.h"
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
#include <GL/gl.h>
#include <GL/glx.h>
#include "xwindow.h"
#include "x11glue.h"
#include "pure_C.h"

/* These qualify this UserInterface */
#define NAME "x11"
#define DESCRIPTION "Simple X11 user interface using keypresses and mouse movements"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
static int globalSync = 0; // this used to be a user option, now it's  static.


/* This structure allows finer control over the UserInterface,
 * based on which renderer will eventually be used.  It can be 
 * customized for exactly the features this particular UserInterface
 * needs to have handled by glue routines.
 */
 struct RendererSpecificGlue{
    /* Each renderer needs to be able to choose its own visual,
     * so it can guarantee that a visual that can support its
     * form of rendering is chosen.
     */
    XVisualInfo *(*ChooseVisual)(Display *display, int screenNumber);

   /* The UserInterface passes on the responsibility of reporting
     * status directly to the Glue routines, who will render 
     * the status text directly into their own windows.
     */
    void (*DrawString)(Canvas *canvas, int row, int column, const char *str);

    /* These are standard configuration thingies */
    void (*BeforeRender)(Canvas *canvas);
    void (*AfterRender)(Canvas *canvas);
    void (*SwapBuffers)(Canvas *canvas);
} ;



/* This function is called to initialize an already-allocated Canvas.
 * The Glue information is already copied into place.
 */
XWindow::XWindow(Canvas *canvas,  ProgramOptions *options, qint32 uiData):
  mOptions(options), mCanvas(canvas), display(NULL), 
  visInfo(NULL), screenNumber(0), window(0), isSubWindow(0), 
  fontInfo(NULL), fontHeight(0),  mShowCursor(true) {
  ECHO_FUNCTION(5);
  RendererSpecificGlue *rendererGlue = 
    GetRendererSpecificGlueByName(options->mOldRenderer->name); 
  Window parentWindow = (Window) uiData;
  const Rectangle *geometry = &options->geometry;
  int decorations = options->decorations;
  QString suggestedName = options->suggestedTitle;
  
  Screen *screen;
  XSetWindowAttributes windowAttrs;
  unsigned long windowAttrsMask;
  XSizeHints sizeHints;
  const int winBorder = 0;
  int x, y, width, height;
  int required_x_margin, required_y_margin;
      
  display = XOpenDisplay(options->displayName.toAscii());
  if (!display) {
    QString err("cannot open display '%1'"); 
    ERROR(err.arg(options->displayName));
    return ;
  }
  
  /* If the user asked for synchronous behavior, provide it */
  if (globalSync) {
    (void) XSynchronize(display, True);
  }
  
  if (parentWindow)
    isSubWindow = 1;
  
  /* Get the screen and do some sanity checks */
  screenNumber = DefaultScreen(display);
  screen = ScreenOfDisplay(display, screenNumber);
  
  
  /* if geometry is don't care and decorations flag is off -- then set window to max screen extents */
  canvas->screenWidth = WidthOfScreen(screen);
  if (geometry->width != DONT_CARE) 
    width = geometry->width;
  else {
    if(decorations) 
      width = DEFAULT_WIDTH;
    else 
      width =  canvas->screenWidth;
  }
  
  canvas->screenHeight = HeightOfScreen(screen);
  if (geometry->height != DONT_CARE) 
    height = geometry->height;
  else {
    if(decorations)
      height = DEFAULT_HEIGHT;
    else
      height =  canvas->screenHeight; 
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
  
  
  
  /* Each renderer has its own way of choosing a visual; pass off the
   * choice of visual to the glue routine.
   */
  visInfo = rendererGlue->ChooseVisual( display, screenNumber );
  if (visInfo == NULL) {
    XCloseDisplay(display);
    ERROR("Could not get visInfo in XWindow constructor.\n"); 
    return ;
  }
  
  /* Set up desired window attributes */
  colormap = XCreateColormap(display, RootWindow(display, screenNumber),
                             visInfo->visual, AllocNone);
  windowAttrsMask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
  windowAttrs.background_pixel = 0x0;
  windowAttrs.border_pixel = 0x0;
  windowAttrs.colormap = colormap;
  windowAttrs.event_mask = StructureNotifyMask | ExposureMask;
  if (parentWindow) {
    /* propogate mouse/keyboard events to parent */
  }
  else {
    windowAttrs.event_mask |= (KeyPressMask | ButtonPressMask |
                               ButtonReleaseMask | ButtonMotionMask);
  }
  
  if (!parentWindow)
    parentWindow = RootWindow(display, screenNumber);
  
  window = XCreateWindow(display, parentWindow,
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
  
  /* Prepare our Canvas structure that we can return to the caller */
  canvas->width = width;
  canvas->height = height;
  canvas->depth = visInfo->depth;
  
  /*
    XXX Perhaps these two should belong to the Renderer, not to the
    UserInterface?
  */
  canvas->ResizePtr = ResizeXWindow;
  canvas->MovePtr = MoveXWindow;
  canvas->DrawStringPtr = rendererGlue->DrawString;
  canvas->BeforeRenderPtr = rendererGlue->BeforeRender;
  canvas->AfterRenderPtr = rendererGlue->AfterRender;
  canvas->SwapBuffersPtr = rendererGlue->SwapBuffers;
  
  
  return ;
} // END CONSTRUCTOR for XWindow

 
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

//======================================================   
// END XWindow struct
//======================================================   
/*
 * Resize the Canvas's X window to given 
 */
void ResizeXWindow(Canvas *canvas, int newWidth, int newHeight, int cameFromX){
  ECHO_FUNCTION(5);
  if (cameFromX) {
    canvas->height = newHeight; 
    canvas->width = newWidth; 
    return; 
  }
    XWindowChanges values;
    unsigned int mask;

    bb_assert(canvas);
    bb_assert(newWidth >= 0);
    bb_assert(newHeight >= 0);

    if (canvas->width == newWidth && canvas->height == newHeight)
        return;

    /* X windows must be at least 1x1 */
    if (newWidth == 0)
       newWidth = 1;
    if (newHeight == 0)
       newHeight = 1;


    values.width = newWidth;
    values.height = newHeight;
    mask = CWWidth | CWHeight;
    XConfigureWindow(canvas->mXWindow->display, canvas->mXWindow->window, mask, &values);
    /* Force sync, in case we get no events (dmx) */
    XSync(canvas->mXWindow->display, 0);

    canvas->width = newWidth;
    canvas->height = newHeight;

    return; 
}


/*
 * Move the Canvas's X window to given 
 * Generally only used when using DMX.
 */
void MoveXWindow(Canvas *canvas, int newX, int newY, int cameFromX) {
  ECHO_FUNCTION(5);
  //cerr << "MoveXWindow"<<endl;
  if (cameFromX) {
    canvas->XPos = newX; 
    canvas->YPos = newY; 
    return; 
  }
   bb_assert(canvas);
    //bb_assert(canvas->mXWindow->isSubWindow);
    XMoveWindow(canvas->mXWindow->display, canvas->mXWindow->window, newX, newY);
    canvas->XPos = newX;  
    canvas->YPos = newY; 
    /* Force sync, in case we get no events (dmx) */
    XSync(canvas->mXWindow->display, 0);
}


/*
 * Helper macro used by the renderers
 */
#define EVENT_TYPE_STRING(x) (\
    x==MapNotify?"MapNotify":\
    x==UnmapNotify?"UnmapNotify":\
    x==ReparentNotify?"ReparentNotify":\
    "unknown")


/*
 * Get the next event from the given Canvas's X window, or
 * the idle event (MOVIE_NONE) if no event is present and
 * we aren't supposed to block waiting for input.
 */
 void
 GetXEvent(Canvas *canvas, int block, MovieEvent *movieEvent)
{
  bool resize=false, move=false; 
    XEvent event;
    Display *dpy = canvas->mXWindow->display;
    static  long oldWidth = -1, oldHeight = -1, 
	  oldX = -1, oldY = -1; 
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
            if (oldHeight != event.xconfigure.height || 
				oldWidth != event.xconfigure.width) {
			  resize = true; 
			  oldHeight = event.xconfigure.height;
			  oldWidth = event.xconfigure.width;
 			  movieEvent->eventType = MOVIE_RESIZE;
			}
			
			if (oldX != event.xconfigure.x || oldY != event.xconfigure.y) {
			  move = true; 
			  oldX = event.xconfigure.x;
			  oldY = event.xconfigure.y;
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
                          canvas->mXWindow->ToggleCursor(); 
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
 void
CloseXWindow(Canvas *canvas)
{
  ECHO_FUNCTION(5);
   if (canvas != NULL) {

        /* Give the Glue routines a chance to free themselves */
        if (canvas->mXWindow != NULL) {
            /* Give the Glue routines a chance to free themselves */
            XDestroyWindow(canvas->mXWindow->display, canvas->mXWindow->window);
            XFreeFont(canvas->mXWindow->display, canvas->mXWindow->fontInfo);
            XFree(canvas->mXWindow->visInfo);
            XFreeColormap(canvas->mXWindow->display, canvas->mXWindow->colormap);
            XSync(canvas->mXWindow->display, 0);
            XCloseDisplay(canvas->mXWindow->display);
            free(canvas->mXWindow);
            
        }
    }
}



/***********************************************************************/
/* Glue routines and data for the OpenGL renderers
 */

static void glBeforeRender(Canvas *canvas)
{
    Bool rv;
    glRenderer *renderer = dynamic_cast<glRenderer*>(canvas->mRenderer); 
    rv = glXMakeCurrent(canvas->mXWindow->display, canvas->mXWindow->window, renderer->context);
    if (rv == False) {
        WARNING("couldn't make graphics context current before rendering");
    }
}

static void glSwapBuffers(Canvas *canvas)
{
     glXSwapBuffers(canvas->mXWindow->display, canvas->mXWindow->window);
}

static XVisualInfo *glChooseVisual(Display *display, int screenNumber)
{
  DEBUGMSG("glChooseVisual (no stereo)"); 
    static int attributes[] = {
        GLX_USE_GL, GLX_RGBA, GLX_DOUBLEBUFFER,
        GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
        None
    };
    XVisualInfo *visualInfo;

    visualInfo = glXChooseVisual(display, screenNumber, attributes);
    if (visualInfo == NULL) {
        ERROR("cannot find a GLX visual on %s to create new OpenGL window",
              DisplayString(display));
        return NULL;
    }
    return visualInfo;
}


static XVisualInfo *glStereoChooseVisual(Display *display, int screenNumber)
{
  DEBUGMSG("glStereoChooseVisual"); 
    static int attributes[] = {
      GLX_USE_GL, GLX_RGBA, GLX_DOUBLEBUFFER, GLX_STEREO,
        GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
        None
    };
    XVisualInfo *visualInfo;

    visualInfo = glXChooseVisual(display, screenNumber, attributes);
    if (visualInfo == NULL) {
        ERROR("cannot find a GLX stereo visual on %s to create new OpenGL window",
              DisplayString(display));
        return NULL;
    }

    return visualInfo;
}

/* This can only be implemented in the "Glue" because it depends on the
 * window system parameters.
 */
static void glDrawString(Canvas *canvas, int row, int column, const char *str)
{
    const int x = (column + 1) * canvas->mXWindow->fontHeight;
    const int y = (row + 1) * canvas->mXWindow->fontHeight;
    glPushAttrib(GL_CURRENT_BIT);
    glBitmap(0, 0, 0, 0, x, canvas->height - y - 1, NULL);
    glCallLists(strlen(str), GL_UNSIGNED_BYTE, (GLubyte *) str);
    glPopAttrib();
}

/***********************************************************************/
/* Glue routines and data for the X11 renderer
 */

static void x11SwapBuffers(Canvas *canvas)
{
  x11Renderer *renderer = dynamic_cast<x11Renderer*>(canvas->mRenderer); 
  
  if (renderer->backBuffer) {
    /* If we're using DBE */
    XdbeSwapInfo swapInfo;
    swapInfo.swap_window = canvas->mXWindow->window;
    swapInfo.swap_action = renderer->mSwapAction;
    XdbeSwapBuffers(canvas->mXWindow->display, &swapInfo, 1);
    /* Force sync, in case we get no events (dmx) */
    XSync(canvas->mXWindow->display, 0);
  }
}



RendererSpecificGlue x11RendererSpecificGlue = {
  pureC_x11ChooseVisual,
  NULL,               /* use Renderer's DrawString routine */
  NULL,               /* no BeforeRender routine necessary */
  NULL,               /* no AfterRender routine necessary */
  x11SwapBuffers
};


RendererSpecificGlue glRendererSpecificGlue = {
  glChooseVisual,
  glDrawString,
  glBeforeRender,
  NULL,               /* no AfterRender routine necessary */
  glSwapBuffers
};


RendererSpecificGlue glStereoRendererSpecificGlue = {
  glStereoChooseVisual,
  glDrawString,
  glBeforeRender,
  NULL,               /* no AfterRender routine necessary */
  glSwapBuffers
};

#ifdef USE_DMX

/***********************************************************************/
/* Glue routines and data for the DMX renderer
 */



RendererSpecificGlue dmxRendererSpecificGlue = {
  pureC_x11ChooseVisual,            /* same as X11 */
  NULL,                       /* use Renderer's DrawString routine */
  NULL,                       /* no BeforeRender routine necessary */
  NULL,                       /* no AfterRender routine necessary */
  NULL,                       /* use Renderer's SwapBuffers routine */
};

#endif

/***********************************************************************/
/* Finally, here is the actual definition of the X11 user interface,
 * including a list of supported renderers with the glue required to
 * support them, and our own initialization support.  Note that the
 * order of "glue" is important - if a user interface is specified,
 * but no renderer, the first one will be chosen.
 */

RendererSpecificGlue *GetRendererSpecificGlueByName(QString name) {
   if (name == "")  return &glRendererSpecificGlue; 

  if (name == "x11") {
    fprintf(stderr, "Error:  x11 renderer is no longer supported.\n"); 
    exit(1); 
  }

  if (name == "gl") return &glRendererSpecificGlue; 
  if (name == "gl_stereo") return &glStereoRendererSpecificGlue; 
  if (name == "gltexture") return &glRendererSpecificGlue; // same as "gl"
#ifdef USE_DMX
  if (name == "dmx") return &dmxRendererSpecificGlue; 
#endif
  return NULL; 
 
}
