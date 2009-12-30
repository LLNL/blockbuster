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
static int globalSync = 0; // this used to be a user option, now it's just static.

/* X11 window system info, not used outside of xwindow.cpp */
 struct WindowInfo{
    Display *display;
    XVisualInfo *visInfo;
    int screenNumber;
    Window window;        /* the window we're really drawing into */
    int isSubWindow;          /* will be true if DMX slave */
    XFontStruct *fontInfo;
    int fontHeight;
    Colormap colormap;
    void (*DestroyGlue)(Canvas *canvas);
   bool mShowCursor; 
    /* These fields are only appropriate to some renderers.
     * To be completely pure, we should pull them out of this
     * structure and allow the renderer glue to create and initialize
     * their own data structures.  For simplicity, they're included
     * here.
     */
    struct {
        GC gc;
        XdbeBackBuffer backBuffer;
    } x11;
    struct {
        GLXContext context;
        GLuint fontBase;
    } glx;

} ;

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

    /* Each renderer needs to be able to finish the initialization;
     * in particular, renderers supported by this user interface may
     * have their own unique supported image formats.
     */
    MovieStatus (*FinishInitialization)(Canvas *canvas, const ProgramOptions *options);

    /* Each glue interface gets an opportunity to undo whatever
     * it did during initialization.
     */
   void (*DestroyGlue)(Canvas *canvas);

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

static WindowInfo *sWindowInfo = NULL; 

/* This utility function converts a raw mask into a mask and shift */
int ComputeShift(unsigned long mask)
{
    register int shiftCount = 0;
    while (mask != 0) {
	mask >>= 1;
	shiftCount++;
    }
    return shiftCount;
}


/* Possible swap actions: 
    {"undefined", XdbeUndefined, "back buffer becomes undefined on swap"},
-    {"background", XdbeBackground, "back buffer is cleared to window background on swap"},
-    {"untouched", XdbeUntouched, "back buffer is contents of front buffer on swap"},
-    {"copied", XdbeCopied, "back buffer is held constant on swap"},
*/ 
static XdbeSwapAction globalSwapAction = XdbeBackground;


//======================================================   
void XWindow_ShowCursor(bool show) {
  DEBUGMSG("Show cursor: %d\n", show); 
  //cerr << "Show cursor: "<< show; 

  if (!sWindowInfo)  return; 
  
  if (!show) {
    /* make a blank cursor */
    Pixmap blank;
    XColor dummy;
    char data[1] = {0};
    Cursor cursor;
    Display *dpy = sWindowInfo->display; 
    Window win = sWindowInfo->window; 
    
    blank = XCreateBitmapFromData (dpy, win, data, 1, 1); 
    if(blank == None) fprintf(stderr, "error: out of memory.\n");
    cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap (dpy, blank);
    
    
    //this makes you the cursor. then set it using this function
    XDefineCursor(dpy,  win,  cursor);
  } else {
    if (!sWindowInfo->mShowCursor) {
      XUndefineCursor(sWindowInfo->display, sWindowInfo->window);
    }
  }
  sWindowInfo->mShowCursor = show; 
  return; 
}

//======================================================   
void XWindow_ToggleCursor(void) {
  ECHO_FUNCTION(5);
  XWindow_ShowCursor(!sWindowInfo->mShowCursor); 
  return; 
}


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
    XConfigureWindow(sWindowInfo->display, sWindowInfo->window, mask, &values);
    /* Force sync, in case we get no events (dmx) */
    XSync(sWindowInfo->display, 0);

    canvas->width = newWidth;
    canvas->height = newHeight;
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
    //bb_assert(sWindowInfo->isSubWindow);
    XMoveWindow(sWindowInfo->display, sWindowInfo->window, newX, newY);
    canvas->XPos = newX;  
    canvas->YPos = newY; 
    /* Force sync, in case we get no events (dmx) */
    XSync(sWindowInfo->display, 0);
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
 GetXEvent(Canvas */*canvas*/, int block, MovieEvent *movieEvent)
{
  bool resize=false, move=false; 
    XEvent event;
    Display *dpy = sWindowInfo->display;
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
                          XWindow_ToggleCursor(); 
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
        if (sWindowInfo != NULL) {
            /* Give the Glue routines a chance to free themselves */
            if (sWindowInfo->DestroyGlue != NULL) {
                sWindowInfo->DestroyGlue(canvas);
            }
            XDestroyWindow(sWindowInfo->display, sWindowInfo->window);
            XFreeFont(sWindowInfo->display, sWindowInfo->fontInfo);
            XFree(sWindowInfo->visInfo);
            XFreeColormap(sWindowInfo->display, sWindowInfo->colormap);
            XSync(sWindowInfo->display, 0);
            XCloseDisplay(sWindowInfo->display);
            free(sWindowInfo);
            
        }
    }
}
 
/*
 * Helper to remove window decorations
 */
static void set_mwm_border( Display *dpy, Window w, unsigned long flags )
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
   motif_hints.decorations = flags;

   /* get the atom for the property */
   prop = XInternAtom( dpy, "_MOTIF_WM_HINTS", True );
   if (!prop) {
      /* something went wrong! */
      return;
   }

   /* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
   proptype = prop;

   XChangeProperty( dpy, w,                         /* display, window */
                    prop, proptype,                 /* property, type */
                    32,                             /* format: 32-bit datums */
                    PropModeReplace,                /* mode */
                    (unsigned char *) &motif_hints, /* data */
                    PROP_MOTIF_WM_HINTS_ELEMENTS    /* nelements */
                  );
}

void XWindow_SetTitle(QString title) {
  ECHO_FUNCTION(5);
  XStoreName(sWindowInfo->display, sWindowInfo->window, 
             title.toAscii().data()); 
  return; 
}

/* This function is called to initialize an already-allocated Canvas.
 * The Glue information is already copied into place.
 */
MovieStatus xwindow_Initialize(Canvas *canvas, const ProgramOptions *options,
                               qint32 uiData)
{
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
    MovieStatus rv;

    if ((sWindowInfo = (WindowInfo *)calloc(1, sizeof(WindowInfo))) == NULL) { 
        ERROR("could not allocate window-specific info");
        return MovieFailure;
    }

    sWindowInfo->mShowCursor = true;  

    sWindowInfo->display = XOpenDisplay(options->displayName.toAscii());
    if (!sWindowInfo->display) {
	  QString err("cannot open display '%1'"); 
	  ERROR(err.arg(options->displayName));
	  free(sWindowInfo);
	  return MovieFailure;
    }

    /* If the user asked for synchronous behavior, provide it */
    if (globalSync) {
        (void) XSynchronize(sWindowInfo->display, True);
    }

    if (parentWindow)
        sWindowInfo->isSubWindow = 1;

    /* Get the screen and do some sanity checks */
    sWindowInfo->screenNumber = DefaultScreen(sWindowInfo->display);
    screen = ScreenOfDisplay(sWindowInfo->display, sWindowInfo->screenNumber);


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
    sWindowInfo->visInfo = rendererGlue->ChooseVisual(
        sWindowInfo->display,
        sWindowInfo->screenNumber
    );
    if (sWindowInfo->visInfo == NULL) {
        XCloseDisplay(sWindowInfo->display);
        free(sWindowInfo);
        return MovieFailure;
    }

    /* Set up desired window attributes */
    sWindowInfo->colormap = XCreateColormap(sWindowInfo->display,
               RootWindow(sWindowInfo->display, sWindowInfo->screenNumber),
               sWindowInfo->visInfo->visual, AllocNone);
    windowAttrsMask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
    windowAttrs.background_pixel = 0x0;
    windowAttrs.border_pixel = 0x0;
    windowAttrs.colormap = sWindowInfo->colormap;
    windowAttrs.event_mask = StructureNotifyMask | ExposureMask;
    if (parentWindow) {
       /* propogate mouse/keyboard events to parent */
    }
    else {
       windowAttrs.event_mask |= (KeyPressMask | ButtonPressMask |
                                  ButtonReleaseMask | ButtonMotionMask);
    }

    if (!parentWindow)
       parentWindow = RootWindow(sWindowInfo->display, sWindowInfo->screenNumber);

    sWindowInfo->window = XCreateWindow(sWindowInfo->display, parentWindow,
            x, y, width, height,
            winBorder, sWindowInfo->visInfo->depth, InputOutput,
            sWindowInfo->visInfo->visual, windowAttrsMask, &windowAttrs);

    DEBUGMSG("created window 0x%x", sWindowInfo->window);

    /* Pass some information along to the window manager to keep it happy */
    sizeHints.flags = USSize;
    sizeHints.width = width;
    sizeHints.height = height;
    if (geometry->x != DONT_CARE && geometry->y != DONT_CARE) {
       sizeHints.x = geometry->x;
       sizeHints.y = geometry->y;
       sizeHints.flags |= USPosition;
    }

  
    XSetNormalHints(sWindowInfo->display, sWindowInfo->window, &sizeHints);
        

    XWindow_SetTitle(suggestedName); 
    XSetStandardProperties(sWindowInfo->display, sWindowInfo->window, 
						   suggestedName.toAscii(), suggestedName.toAscii(), 
                           None, (char **)NULL, 0, &sizeHints);


    if (!decorations) 
       set_mwm_border(sWindowInfo->display, sWindowInfo->window, 0);

    /* Bring it up; then wait for it to actually get here. */
    XMapWindow(sWindowInfo->display, sWindowInfo->window);
    XSync(sWindowInfo->display, 0);
    /*
    XIfEvent(sWindowInfo->display, &event, WaitForWindowOpen, (XPointer) sWindowInfo);
    */

    /* The font.  All renderers that attach to this user interface will use
     * this font for rendering status information directly into the rendering
     * window.
     */
    sWindowInfo->fontInfo = XLoadQueryFont(sWindowInfo->display, 
										  options->fontName.toAscii());
    if (!sWindowInfo->fontInfo) {
	  QString warning("Couldn't load font %s, trying %s");
	  WARNING(warning.arg(options->fontName).arg(DEFAULT_X_FONT));

        sWindowInfo->fontInfo = XLoadQueryFont(sWindowInfo->display,
											  DEFAULT_X_FONT);
        if (!sWindowInfo->fontInfo) {
            WARNING("Couldn't load font %s", DEFAULT_X_FONT);
            XFree(sWindowInfo->visInfo);
            XFreeColormap(sWindowInfo->display, sWindowInfo->colormap);
            XCloseDisplay(sWindowInfo->display);
            free(sWindowInfo);
            return MovieFailure;
        }
    }
    sWindowInfo->fontHeight = sWindowInfo->fontInfo->ascent + sWindowInfo->fontInfo->descent;

    /* Prepare our Canvas structure that we can return to the caller */
    canvas->width = width;
    canvas->height = height;
    canvas->depth = sWindowInfo->visInfo->depth;

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

    sWindowInfo->DestroyGlue = rendererGlue->DestroyGlue;

    /* The Glue routines will finish initialization here; in particular, they need
     * to prepare to use the font (above), and to set up the required image format
     * (as each requires an independent format).
     */
    rv = rendererGlue->FinishInitialization(canvas, options);
    if (rv != MovieSuccess) {
        XFreeFont(sWindowInfo->display, sWindowInfo->fontInfo);
        XFree(sWindowInfo->visInfo);
        XFreeColormap(sWindowInfo->display, sWindowInfo->colormap);
        XCloseDisplay(sWindowInfo->display);
        free(sWindowInfo);
        return rv;
    }

    return MovieSuccess;
}

/***********************************************************************/
/* This is an ugly little routine that gives access to the Window
 * and Display parameters.
 */
void GetX11UserInterfaceInfo(void */*dataPtr*/, Display **display, Window *window)
{

    *display = sWindowInfo->display;
    *window = sWindowInfo->window;
}

/***********************************************************************/
/* Glue routines and data for the OpenGL renderers
 */

static void glBeforeRender(Canvas *)
{
    Bool rv;

    rv = glXMakeCurrent(sWindowInfo->display, sWindowInfo->window, sWindowInfo->glx.context);
    if (rv == False) {
        WARNING("couldn't make graphics context current before rendering");
    }
}

static void glSwapBuffers(Canvas *)
{
     glXSwapBuffers(sWindowInfo->display, sWindowInfo->window);
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
    const int x = (column + 1) * sWindowInfo->fontHeight;
    const int y = (row + 1) * sWindowInfo->fontHeight;
    glPushAttrib(GL_CURRENT_BIT);
    glBitmap(0, 0, 0, 0, x, canvas->height - y - 1, NULL);
    glCallLists(strlen(str), GL_UNSIGNED_BYTE, (GLubyte *) str);
    glPopAttrib();
}

static MovieStatus glFinishInitialization(Canvas *canvas, const ProgramOptions *options)
{
    Bool rv;
    Font id = sWindowInfo->fontInfo->fid;
    unsigned int first = sWindowInfo->fontInfo->min_char_or_byte2;
    unsigned int last = sWindowInfo->fontInfo->max_char_or_byte2;

    /* All GL rendering in X11 requires a glX context. */
    sWindowInfo->glx.context = glXCreateContext(sWindowInfo->display, sWindowInfo->visInfo,
                                            NULL, GL_TRUE);
    if (!sWindowInfo->glx.context) {
        ERROR("couldn't create GLX context");
        return MovieFailure;
    }

    rv = glXMakeCurrent(sWindowInfo->display, sWindowInfo->window, sWindowInfo->glx.context);
    if (rv == False) {
        ERROR("couldn't make graphics context current");
        glXDestroyContext(sWindowInfo->display, sWindowInfo->glx.context);
        return MovieFailure;
    }

    DEBUGMSG("GL_RENDERER = %s", (char *) glGetString(GL_RENDERER));

    /* OpenGL display list font bitmaps */
    sWindowInfo->glx.fontBase = glGenLists((GLuint) last + 1);
    if (!sWindowInfo->glx.fontBase) {
        ERROR("Unable to allocate display lists for fonts");
        glXDestroyContext(sWindowInfo->display, sWindowInfo->glx.context);
        return MovieFailure;
    }

    glXUseXFont(id, first, last - first + 1, sWindowInfo->glx.fontBase + first);
    glListBase(sWindowInfo->glx.fontBase);

    /* Specify our required format.  For OpenGL, always assume we're
     * getting 24-bit RGB pixels.
     */
    canvas->requiredImageFormat.scanlineByteMultiple = 1;
    canvas->requiredImageFormat.rowOrder = ROW_ORDER_DONT_CARE;
    canvas->requiredImageFormat.byteOrder = MSB_FIRST;
    canvas->requiredImageFormat.bytesPerPixel = 3;

	options=NULL; //shut up compiler
    return MovieSuccess;
}

/* Clean up after anything we did during initialization */
static void glDestroyGlue(Canvas *)
{
 
    glXDestroyContext(sWindowInfo->display, sWindowInfo->glx.context);
}

/***********************************************************************/
/* Glue routines and data for the X11 renderer
 */

static void x11SwapBuffers(Canvas *)
{
    if (sWindowInfo->x11.backBuffer) {
        /* If we're using DBE */
        XdbeSwapInfo swapInfo;
        swapInfo.swap_window = sWindowInfo->window;
        swapInfo.swap_action = globalSwapAction;
        XdbeSwapBuffers(sWindowInfo->display, &swapInfo, 1);
        /* Force sync, in case we get no events (dmx) */
        XSync(sWindowInfo->display, 0);
    }
}


static MovieStatus x11FinishInitialization(Canvas *canvas, const ProgramOptions *)
{
    X11RendererGlue *glueInfo;

    /* The X11 Renderer will require this structure to be present in gluePrivateData,
     * to give it its rendering parameters
     */
    glueInfo = (X11RendererGlue *)calloc(1, sizeof(X11RendererGlue));
    if (glueInfo == NULL) {
        ERROR("Cannot allocate X11 renderer info");
        return MovieFailure;
    }

    /* This graphics context and font will be used for rendering status messages,
     * and as such are owned here, by the UserInterface.
     */
    sWindowInfo->x11.gc = XCreateGC(sWindowInfo->display, sWindowInfo->window, 0, NULL);
    XSetFont(sWindowInfo->display, sWindowInfo->x11.gc, sWindowInfo->fontInfo->fid);
    XSetForeground(sWindowInfo->display, sWindowInfo->x11.gc,
           WhitePixel(sWindowInfo->display, sWindowInfo->screenNumber));

    sWindowInfo->x11.backBuffer = XdbeAllocateBackBufferName(sWindowInfo->display,
                                        sWindowInfo->window, globalSwapAction);

    glueInfo->display = sWindowInfo->display;
    glueInfo->visual = sWindowInfo->visInfo->visual;
    glueInfo->depth = sWindowInfo->visInfo->depth;
    if (sWindowInfo->x11.backBuffer) {
        glueInfo->doubleBuffered = 1;
        glueInfo->drawable = sWindowInfo->x11.backBuffer;
    }
    else {
        glueInfo->doubleBuffered = 0;
        glueInfo->drawable = sWindowInfo->window;
    }
    glueInfo->gc = sWindowInfo->x11.gc;
    glueInfo->fontHeight = sWindowInfo->fontHeight;
    canvas->gluePrivateData = glueInfo;

    /* Specify our required format.  Note that 24-bit X11 images require
     * *4* bytes per pixel, not 3.
     */
    if (sWindowInfo->visInfo->depth > 16) {
        canvas->requiredImageFormat.bytesPerPixel = 4;
    }
    else if (sWindowInfo->visInfo->depth > 8) {
        canvas->requiredImageFormat.bytesPerPixel = 2;
    }
    else {
        canvas->requiredImageFormat.bytesPerPixel = 1;
    }
    canvas->requiredImageFormat.scanlineByteMultiple = BitmapPad(sWindowInfo->display)/8;

    /* If the bytesPerPixel value is 3 or 4, we don't need these;
     * but we'll put them in anyway.
     */
    canvas->requiredImageFormat.redShift = ComputeShift(sWindowInfo->visInfo->visual->red_mask) - 8;
    canvas->requiredImageFormat.greenShift = ComputeShift(sWindowInfo->visInfo->visual->green_mask) - 8;
    canvas->requiredImageFormat.blueShift = ComputeShift(sWindowInfo->visInfo->visual->blue_mask) - 8;
    canvas->requiredImageFormat.redMask = sWindowInfo->visInfo->visual->red_mask;
    canvas->requiredImageFormat.greenMask = sWindowInfo->visInfo->visual->green_mask;
    canvas->requiredImageFormat.blueMask = sWindowInfo->visInfo->visual->blue_mask;
    canvas->requiredImageFormat.byteOrder = ImageByteOrder(sWindowInfo->display);
    canvas->requiredImageFormat.rowOrder = TOP_TO_BOTTOM;


    return MovieSuccess;
}

static void x11DestroyGlue(Canvas *canvas)
{
    X11RendererGlue *glueInfo = (X11RendererGlue *)canvas->gluePrivateData;

    XFreeGC(sWindowInfo->display, sWindowInfo->x11.gc);

    free(glueInfo);
}


RendererSpecificGlue x11RendererSpecificGlue = {
    pureC_x11ChooseVisual,
    x11FinishInitialization,
    x11DestroyGlue,
    NULL,               /* use Renderer's DrawString routine */
    NULL,               /* no BeforeRender routine necessary */
    NULL,               /* no AfterRender routine necessary */
    x11SwapBuffers
};


RendererSpecificGlue glRendererSpecificGlue = {
    glChooseVisual,
    glFinishInitialization,
    glDestroyGlue,
    glDrawString,
    glBeforeRender,
    NULL,               /* no AfterRender routine necessary */
    glSwapBuffers
};


RendererSpecificGlue glStereoRendererSpecificGlue = {
    glStereoChooseVisual,
    glFinishInitialization,
    glDestroyGlue,
    glDrawString,
    glBeforeRender,
    NULL,               /* no AfterRender routine necessary */
    glSwapBuffers
};

/*
static RendererGlue GLGlue = {
    &glRenderer,
    &GLRendererSpecificGlue
};

static RendererGlue GLStereoGlue = {
    &glRendererStereo,
    &GLStereoRendererSpecificGlue
};


static RendererGlue GLTextureGlue = {
    &glTextureRenderer,
    &GLRendererSpecificGlue
};
*/ 
#ifdef USE_DMX

/***********************************************************************/
/* Glue routines and data for the DMX renderer
 */


static MovieStatus dmxFinishInitialization(Canvas *canvas, const ProgramOptions *options)
{
     DMXRendererGlue *glueInfo;

    /* The DMX Renderer will require this structure to be present in gluePrivateData,
     * to give it its rendering parameters
     */
    glueInfo = (DMXRendererGlue *)calloc(1, sizeof(DMXRendererGlue));
    if (glueInfo == NULL) {
        ERROR("Cannot allocate DMX renderer info");
        return MovieFailure;
    }

    /* This graphics context and font will be used for rendering status messages,
     * and as such are owned here, by the UserInterface.
     */
    /* copy some fileds form sWindowInfo to glueInfo */
    glueInfo->display = sWindowInfo->display;
    glueInfo->window = sWindowInfo->window;
    glueInfo->fontHeight = sWindowInfo->fontHeight;
    glueInfo->frameCacheSize = options->frameCacheSize;
    glueInfo->readerThreads = options->readerThreads;

    canvas->gluePrivateData = glueInfo;

    /* Uniquely, DMX doesn't have any required format; its Render
     * method doesn't actually load any imagery.
     */

    return MovieSuccess;
}

static void dmxDestroyGlue(Canvas *canvas)
{
    DMXRendererGlue *glueInfo = (DMXRendererGlue *)canvas->gluePrivateData;

    free(glueInfo);
}

RendererSpecificGlue dmxRendererSpecificGlue = {
  pureC_x11ChooseVisual,            /* same as X11 */
  dmxFinishInitialization,
  dmxDestroyGlue,
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
/* RendererGlue *x11_supportedRendererGlueChoices[] = {
  &GLGlue,
  &GLTextureGlue,
  &X11Glue,
  &GLStereoGlue,
#ifdef USE_DMX
  &DMXGlue,
#endif
  NULL
};

UserInterface x11UserInterface = {
    NAME,
    DESCRIPTION,
    //x11_supportedRendererGlueChoices,
    //xwindow_HandleOptions,
    xwindow_Initialize,
    NULL // no ChooseFile implementation 

};
*/ 

