#ifndef X11RENDERER_H
#define X11RENDERER_H yes

#include "Renderer.h" // not "Renderers.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>

//====================================================================
// x11Renderer CLASS
//======================================================================
class x11Renderer: public NewRenderer {
 public:
  x11Renderer(ProgramOptions *opt, Canvas *canvas, Window parentWindow);

  virtual ~x11Renderer() ;
  
  int ComputeShift(unsigned long mask);

  void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
  void DrawString(int row, int column, const char *str);
 
  // from X11RendererGlue
  Drawable drawable;
  int doubleBuffered;
  // was global: 
  XdbeSwapAction mSwapAction;
  // from WindowInfo struct in xwindow.cpp
  GC gc;
  XdbeBackBuffer backBuffer;

} ;

#endif
