#ifndef X11RENDERER_H
#define X11RENDERER_H yes

#include "Renderer.h" // not "Renderers.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>

//====================================================================
// x11Renderer CLASS
//======================================================================
class x11Renderer: public Renderer {
 public:
  x11Renderer(ProgramOptions *opt, Window parentWindow, 
              BlockbusterInterface *gui, QString name="x11");
  virtual void FinishRendererInit(ProgramOptions *opt);

  virtual ~x11Renderer() ;
  
  int ComputeShift(unsigned long mask);
  virtual XVisualInfo *ChooseVisual(void) {
    return NULL; // does not apply to X11
  }
 
 ImagePtr ScaleImage(ImagePtr image, 
                     int srcX, int srcY, int srcWidth, int srcHeight,
                     int zoomedWidth, int zoomedHeight);
 virtual void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
  virtual void DrawString(int row, int column, const char *str);
  virtual void SwapBuffers(void);
 
  // from X11RendererGlue
  Drawable mDrawable;
  int mDoubleBuffered;
  // was global: 
  XdbeSwapAction mSwapAction;
  // from WindowInfo struct in xwindow.cpp
  GC mGC;
  XdbeBackBuffer mBackBuffer;

} ;

#endif
