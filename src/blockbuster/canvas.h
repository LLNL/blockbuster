#ifndef BLOCKBUSTER_CANVAS_H
#define BLOCKBUSTER_CANVAS_H

#include "common.h"
#include "frames.h"
#include "settings.h"
#include "events.h"

#define SCREEN_X_MARGIN 20
#define SCREEN_Y_MARGIN 40

  /* The more complicated parent of the dynamic Canvas object is the
   * UserInterface.  This is a collection of routines that know how to manage
   * windows, how to get control messages from the user to the application, and how 
   * to report messages from the application back to the user.
   *
   */

  /* The Canvas object is the dynamic rendering object.  It inherits
   * behaviors from both its parent UserInterface object and from its
   * parent Renderer object (both of which are "static", in that they
   * do not create a dynamic object outside of the Canvas).
   */

   struct Canvas {
     Canvas(qint32 parentWindowID, BlockbusterInterface *gui=NULL);

     ~Canvas(); 

    int height;
    int width;
    int screenHeight, screenWidth; /* for when sidcar wants whole screen */ 
    int XPos; 
    int YPos; 
    int depth;
  int threads;
  int cachesize;
     
     BlockbusterInterface *mBlockbusterInterface; 

    /**************************************************************/
    /* These fields are owned and initialized by the Renderer.  It seems like maybe the Renderer is an idea encapsulated by x11.cpp, gl.cpp, or dmxglue.cpp.  
     */

    /* The fundamental operation of the Renderer is to render. So the 
       question is, what the hell is the Render function doing in the canvas? 
       This might be assigned gl_Render (gl.cpp, gl_Initialize), x11_Render (x11.cpp: x11_initialize()), or dmx_Render (dmxglue.cpp, dmx_Initialize()).  The assignment is done 
    */
    void (*Render)(struct Canvas *canvas, int frameNumber,
       const Rectangle *imageRegion,
       int destX, int destY, float zoom, int lod);

   /* The Canvas must manage the lists of frames and image caches
     * filled with frames.  All the Renderers must provide the 
     * methods listed below; typically, they will use the stored
     * FrameList and ImageCache, which is why these two variables
     * have been codified here (rather than require most Renderers
     * to put these variables into privateData).
     */
    FrameList *frameList;
    struct ImageCache *imageCache;
     
    void (*SetFrameList)(struct Canvas *canvas, FrameList *frameList);
    void (*Preload)(struct Canvas *canvas, uint32_t frameNumber,
        const Rectangle *imageRegion, uint32_t levelOfDetail);

    /* The Renderer will get its own chance to release any resources
     * it may have allocated.
     */
    void (*DestroyRenderer)(struct Canvas *canvas);

    /* If the renderer needs a handle on which to hang any privately
     * allocated data, this is the place to do it.
     */
    void *rendererPrivateData;


    /**************************************************************/
    /* The following fields are owned and initialized by the UserInterface.
     */

    /* The best image format for the window or widget configuration
     * created by the UserInterface.  The various FileFormat
     * modules will be told to give us images in this format; if they
     * fail to do so, we'll convert them ourselves (an expensive but
     * functional situation).
     */
    ImageFormat requiredImageFormat;
    /* This renderer function will be called to notify the renderer
     * that a resize has occurred.  Most renderers will not care
     * about such a change, but some that manage subwindows (like
     * DMX) will.
     */
     //void Resize(int newWidth, int newHeight, int cameFromX); 
     void (*Resize)(struct Canvas *canvas, int newWidth, int newHeight, int camefromX);

    /* Only DMX uses this message; it tells a subwindow that it is 
     * supposed to move to a new location relative to its parent
     * window.  DMX uses it to rearrange subwindows after a 
     * resize event.  The "cameFromX" parameter suppresses the 
	 * redundant and bug-prone generation of XMoveWindow() calls 
	 * from moves that themselves originated from X. 
     */
    void (*Move)(struct Canvas *canvas, int newX, int newY, int cameFromX);


    /* This is called if any module wishes to report an error,
     * warning, informational, or debug message.  The modules
     * call through the SYSERROR(), ERROR(), WARNING(), INFO(), 
     * and DEBUGMSG() macros, but ultimately the message comes to here.
     * If the ReportMessage() method is NULL, or returns a
     * STATUS_FAILURE code, the message will be reported to the
     * console instead (as a convenience to the programmer).
     */
    MovieStatus ReportMessage(const char *file, const char *function, 
                              int line, int level, 
                              const char *message);

     void ReportFrameListChange(const FrameList *frameList);
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
     
    /**************************************************************/
    /* This field is shared between the UserInterface and the Renderer;
     * either (or neither!) may choose to implement it.  It is invoked
     * to send a status message to the user.
     *
     * If implemented by a Renderer, it typically renders text
     * over the image already rendered.  A UserInterface may choose
     * to create a specific window or widget to hold any and all such
     * messages.
     *
     * The UserInterface will get the first opportunity to set
     * this value; if it is set, the Renderer's Initialize() method
     * should not override it.
     */
    void (*DrawString)(struct Canvas *canvas, int row, int column, const char *str);


    /**************************************************************/
    /* The following fields are owned and initialized by the 
     * "glue" submodules of the UserInterface.  They are initialized
     * by the UserInterface itself, but typically their values
     * come from the "glue" routines in the UserInterface modules.
     */

    /* Called to swap front/back buffers */
    void (*SwapBuffers)(struct Canvas *canvas);

    /* Called before calling the Render() method, to allow the
     * glue to make sure the surface is ready for rendering (e.g.,
     * by calling glXMakeCurrent(), or a similar widget routine).
     */
    void (*BeforeRender)(struct Canvas *canvas);
    void (*AfterRender)(struct Canvas *canvas);

    void *gluePrivateData;

     int32_t playDirection, startFrame, endFrame, preloadFrames; 
  } ;

/* Canvas creator from canvas.c */
Canvas *CreateCanvas(const UserInterface *userInterface,
                     const int rendererIndex,
                     qint32 parentWindowID);
void DestroyCanvas(Canvas *canvas);
void DefaultSetFrameList(Canvas *canvas, FrameList *frameList);
void CacheSetFrameList(Canvas *canvas, FrameList *frameList);
FrameInfo *GetFrameInfoPtr(Canvas *canvas, int frameNumber);

#endif
