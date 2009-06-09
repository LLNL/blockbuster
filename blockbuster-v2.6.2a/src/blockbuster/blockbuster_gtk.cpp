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
 * This module encapsulates the GTK UserInterface, and includes all the
 * "glue" required by the renderers that the GTK UserInterface supports.
 */

#ifdef USE_GTK
#include "Renderers.h"
#include "dmxglue.h"
#include "gltexture.h"
#include "util.h"
#include "canvas.h"
#include "errmsg.h"
#include "ui.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "remotecommands.h"
#undef signals
#include <gtk/gtk.h>
#include <gdk/gdkx.h>


#include "xwindow.h"

/* These qualify this UserInterface */
#define NAME "gtk"
#define DESCRIPTION "Hybrid GTK/X11 user interface using a GTK toolbar and an X11 window"



void gtk_HandleOptions(int &, char *argv[]) {
  return ;

  // hack to force gtk to respect the display set on the command line
  char **args = (char**)calloc(4, sizeof(char*));
  int argn = 0;
  args[argn++] = argv[0]; 
  char *dpy = getenv("DISPLAY");
  if (dpy)
  {
    args[argn++] = strdup("--display");  
    args[argn++] = strdup(getenv("DISPLAY")); 
  }
  gtk_init(&argn, &args);
    return;
}

/* This structure will hold events collected from the GTK widgets */
struct BufferedEvent {
    MovieEvent event;
    struct BufferedEvent *next;
};


/* This is the information block that we use to preserve state across
 * invocations.
 */
#define NUM_STATUS_LINES 5
struct gtk_UserInterfaceInfo {
  Display *x11Display; 
  Window x11Window, toolboxWindow; 

    /* Event buffers that will collect the events from the GTK interface */
    struct BufferedEvent *firstBufferedEvent, *lastBufferedEvent;

    /* A copy of the Settings that the main program will read from an
     * RC file and pass to us.
     */
    void *settings;

    /* The components of the GTK interface */
    GtkWidget *toolbox;
    vector<GtkWidget *> statusLines;
    GtkWidget *openDialog;
    vector<GtkWidget *> recentFileButtons;
    GtkWidget *recentFileBox;
    GtkWidget *messageDialog;
    GtkWidget *messageDialogTitle;
    GtkWidget *messageDialogText;
    GtkWidget *playOnceButton, *playLoopButton;

    /* These manage the values of the various sliders in the GTK interface */
    GtkAdjustment *frameAdjustment;
    GtkAdjustment *rateAdjustment;
    GtkAdjustment *detailAdjustment;
    GtkAdjustment *zoomAdjustment;
  gtk_UserInterfaceInfo(): 
    firstBufferedEvent(NULL), lastBufferedEvent(NULL), settings(NULL), toolbox(NULL), openDialog(NULL), recentFileBox(NULL), messageDialog(NULL), messageDialogTitle(NULL), messageDialogText(NULL), playOnceButton(NULL), playLoopButton(NULL), frameAdjustment(NULL), rateAdjustment(NULL), detailAdjustment(NULL), zoomAdjustment(NULL) {}
    
} ;

gtk_UserInterfaceInfo *gtkUiInfo = NULL; 

/****************************************************************************/
/* Helper functions.  These manage the event queue within which we'll store
 * GTK events.
 */

typedef enum {
    BufferCollect,
    BufferIndividual
} BufferBehavior;

/* This is a global variable that prevents the interface elements from sending out 
   DisplayLoop events when they are changed programmatically, to prevent feedback
   loops. 
*/ 
static int bufferEnabled = 1;  


static void xGetBufferedEvent(MovieEvent *event)
{
    struct BufferedEvent *bufferedEvent;

    if (gtkUiInfo->firstBufferedEvent == NULL) {
        event->eventType = MOVIE_NONE;
        return;
    }

    bufferedEvent = gtkUiInfo->firstBufferedEvent;
    gtkUiInfo->firstBufferedEvent = bufferedEvent->next;
    if (gtkUiInfo->firstBufferedEvent == NULL) {
        gtkUiInfo->lastBufferedEvent = NULL;
    }
    *event = bufferedEvent->event;
    free(bufferedEvent);
}


void xSetCheckboxActive(const char *checkBoxName, bool activate, bool BufferEvents) {
  if (!checkBoxName || !*checkBoxName) return; 
  //return;
  int saved = bufferEnabled; 
  bufferEnabled = BufferEvents; 
  if (strcmp(checkBoxName, "playLoopButton") == 0) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkUiInfo->playLoopButton), activate);
  }
  else if  (strcmp(checkBoxName, "playOnceButton") == 0) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtkUiInfo->playOnceButton), activate);
  }
  
  bufferEnabled = saved;
  return;
}



/****************************************************************************/
/* Our event processing function will be a hybrid; it will check our
 * list of buffered events for any events requested by the GTK widgets;
 * it will also check the underlying X11 Event mechanism.
 *
 * Note that we never block, even if asked; if we block on one queue,
 * we may miss an event coming on the other one.
 */


#define NONBLOCKING 0
 void xgtk_GetEvent(Canvas *canvas, int block, MovieEvent *movieEvent)
{
    
  //  return ;
    while (1) {
        /* For interactivity, let the main loop run as much as
         * it needs to, to collect user events.
         */
      /*while (gtk_events_pending()) {
        (void) gtk_main_iteration_do(NONBLOCKING);
        }*/

       /* First try to return any user interface events we may have collected */
       //GetBufferedEvent(movieEvent);
       if (movieEvent->eventType != MOVIE_NONE) {
	 /* Got a real event; that's all we need. */
         return;
       }

        /* If we're here, we didn't have a real event from the user interface.
         * Try the X11 event queue instead.  Even if this one returns with
         * MOVIE_NONE, we'll send that back.  Note that we never block; if
         * we did, we'd miss GTK events while waiting for an X11 event
         * to occur.
         */
       GetXEvent(canvas, NONBLOCKING, movieEvent); 
       if (movieEvent->eventType != MOVIE_NONE) {
         /* A real event from X11 */
         return;
       }
       // }

        /* If we get to here, we've tried to get an event from both our queues,
         * but nothing is coming.  If we're not supposed to block, return
         * with the MOVIE_NONE event that's already in place.
         */
        if (!block) {
            return;
        }

        /* If we're here, we're supposed to block.  Give up the processor for
         * a little bit, then continue looking for events.
         */
        usleep((unsigned long) 10); /* delay in microseconds */
    }
}

/* Destruction is also hybrid; we have to take down our widgets before
 * passing control back to the X11 interface.
 */
 void gtk_DestroyUserInterface(Canvas *canvas)
{
    
  return ;

    if (gtkUiInfo != NULL) {
        MovieEvent event;

        /* Remove the widgets, if we have them.  Destroying the top-level
         * widget should destroy all contained widgets.
         */
        if (gtkUiInfo->openDialog) {
            gtk_widget_hide_all(gtkUiInfo->openDialog);
            gtk_widget_destroy(gtkUiInfo->openDialog);
        }
        if (gtkUiInfo->toolbox) {
            gtk_widget_hide_all(gtkUiInfo->toolbox);
            gtk_widget_destroy(gtkUiInfo->toolbox);
        }
        if (gtkUiInfo->messageDialog) {
            gtk_widget_hide_all(gtkUiInfo->messageDialog);
            gtk_widget_destroy(gtkUiInfo->messageDialog);
        }

        /* The adjustments should be destroyed when their widgets are
         * destroyed
         */

        /* With the widgets destroyed, no one should be creating new
         * events; it's okay to clear up the buffered event queue.
         */
        //GetBufferedEvent(&event);
        while (event.eventType != MOVIE_NONE) {
          //GetBufferedEvent(&event);
        }

        /* Destroy our inherited interface */
        CloseXWindow(canvas); 

        /* Free our pile of memory, and go home */
        delete gtkUiInfo;
    }
}


/* This function is called to report a message through the GTK
 * user interface.  Note that we cannot call any of the messaging
 * macros (SYSERROR, ERROR, WARNING, INFO, DEBUG) from within here;
 * that would cause a nice infinite loop.
 */
 MovieStatus gtk_ReportMessage(struct Canvas *, 
        const char *file, const char *function, int line, int level, 
            const char *message)
{
  return MovieSuccess; 

    char *title = NULL;
    char buffer[BLOCKBUSTER_PATH_MAX];
    

    switch(level) {
        case M_SYSERROR: title = "System Error"; break;
        case M_ERROR: title = "Error"; break;
        case M_WARNING: title = "Warning"; break;
        case M_INFO: return MovieFailure; /* let message go to console */
        case M_DEBUG: return MovieFailure; /* let message go to console */
    }


    snprintf(buffer, BLOCKBUSTER_PATH_MAX, "blockbuster - %s", title);
    gtk_window_set_title(GTK_WINDOW(gtkUiInfo->messageDialog), buffer);

    gtk_label_set_text(GTK_LABEL(gtkUiInfo->messageDialogTitle), title);

    snprintf(buffer, BLOCKBUSTER_PATH_MAX, "%s\n\n(%s:%s():%d)",
            message, file, function, line);
    gtk_label_set_text(GTK_LABEL(gtkUiInfo->messageDialogText), buffer);

    gtk_widget_show(gtkUiInfo->messageDialog);
 
    return MovieSuccess;
}

void gtk_DrawString(Canvas *, int row, int /*column*/, const char *str)
{
    
  return ; 

    /* We don't technically draw the string; we just display it in the Status
     * area of our toolbox.
     */
    if (row >= NUM_STATUS_LINES) {
        DEBUGMSG("DrawString got row %d, when only 0-%d are supported",
                row, NUM_STATUS_LINES - 1);
        return;
    }
    gtk_label_set_text(GTK_LABEL(gtkUiInfo->statusLines[row]), str);
    gtk_label_set_justify(GTK_LABEL(gtkUiInfo->statusLines[row]), GTK_JUSTIFY_LEFT);

}
char *ChooseFile(const ProgramOptions *) {
  return NULL; 
}

 void gtk_ShowInterface(Canvas *, int )
{
  return; 
}

 void gtk_ReportFrameListChange(Canvas *, const FrameList *)
{
   
  return; 

     
    
}

 void gtk_ReportFrameChange(Canvas *, int )
{
    
  return; 
 }

/*!
   \parameter behavior -  -1 = play in a loop forever, n>0 = play in a loop n times
*/ 
 void gtk_ReportLoopBehaviorChange(Canvas *, int ) {
   return; 
}

 void gtk_ReportRateChange(Canvas *, float )
{
    
  return; 
}
 void gtk_ReportRateRangeChange(Canvas *, float , float )
{
    
  return; 
}

 void gtk_ReportDetailRangeChange(Canvas *, int , int )
{
    
  return; 
}

 void gtk_ReportDetailChange(Canvas *, int )
{
  return; 

}

 void gtk_ReportZoomChange(Canvas *, float )
{
    
  return; 
}

/* forward declaration -- defined below */ 
extern UserInterface gtkUserInterface;


 MovieStatus gtk_Initialize(Canvas *canvas, const ProgramOptions *options,
           qint32 windowNumber, const RendererSpecificGlue *)
{
    char *rendererName;
    MovieStatus status;
    UserInterface *x11UserInterface;
    int x11RendererIndex;
    RendererGlue *x11Glue;


    /* Before we ask the X11 UserInterface to initialize, we need to be
     * certain of our own renderer; we can't use the given rendererIndex
     * (which is always correct) because our Renderers may be in a different
     * preference order than the X11 Renderers, and using the rendererIndex
     * may give us a different Renderer.
     *
     * We can't use the given rendererName either, because that's the name
     * provided by the end user, and may be NULL.
     *
     * Instead, just look the thing up in our own table.
     */
    rendererName = gtkUserInterface.supportedRenderers[options->rendererIndex]->renderer->name;

    /* Given that, find the correct matching renderer for the X11 user interface.
     * If we fail, it's because of an internal error (we claim to support a
     * renderer that X11 really doesn't support).
     */
    status = MatchUserInterfaceToRenderer("x11", rendererName,
            &x11UserInterface, NULL, &x11RendererIndex);
    if (status != MovieSuccess) {
        ERROR("GTK supports renderer %s but X11 does not", rendererName);
        return status;
    }

    /* Find the appropriate Glue routines to use.  We don't have any Glue of
     * our own; we're just using the X11 stuff.
     */
    x11Glue = x11UserInterface->supportedRenderers[x11RendererIndex];

    /* Get ourselves a big hunk'a'memory to save our own details in. */
    gtkUiInfo = new gtk_UserInterfaceInfo;
    if (gtkUiInfo == NULL) {
        ERROR("Could not allocate GTK user interface information");
        return MovieFailure;
    }

    /* The main program reads in RC files for the player; this
     * file may include settings that affect us.  It'll have the
     * last several files opened, for example; maybe it'll include
     * information on window positioning and such someday.
     */
    gtkUiInfo->settings = options->settings;

    /* Now ask the X11 UserInterface to initialize.  Maybe we'll want later
     * to change some of the options to match our RC settings.
     */
    status = x11UserInterface->Initialize(canvas, options, windowNumber, x11Glue->configurationData);
    if (status != MovieSuccess) {
        return status;
    }

   return MovieSuccess; 

 }



/***********************************************************************/
/* This defines the GTK user interface, including the supported
 * renderers.  Note that we have no "glue" of our own; instead,
 * at initialization time, we let the X11 UserInterface initialize
 * its own "glue". WTF is glue?  
 */

 RendererGlue GLGlue = {&glRenderer, NULL};
 RendererGlue GLTextureGlue = {&glTextureRenderer, NULL};
 RendererGlue X11Glue = {&x11Renderer, NULL};
 RendererGlue GLStereoGlue = {&glRendererStereo, NULL};

#ifdef USE_DMX
 RendererGlue DMXGlue = {&dmxRenderer, NULL};
#endif

/* These are in priority order - if a renderer is not specified, the
 * first one will be chosen.
 */
 RendererGlue *supportedGlue[] = {
    &GLGlue,
    &GLTextureGlue,
    &X11Glue,
    &GLStereoGlue,
#ifdef USE_DMX
    &DMXGlue,
#endif
    NULL
};

UserInterface gtkUserInterface = {
    NAME,
    DESCRIPTION,
    supportedGlue,
    gtk_HandleOptions,
    gtk_Initialize,
    
    ChooseFile
};

#endif
