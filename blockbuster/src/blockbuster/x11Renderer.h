#ifndef X11RENDERER_H
#define X11RENDERER_H yes

#include "Renderer.h" // not "Renderers.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include "x11glue.h"

//====================================================================
// x11Renderer CLASS
//======================================================================
class x11Renderer: public NewRenderer {
 public:
  x11Renderer(ProgramOptions *opt, Canvas *canvas);

  virtual ~x11Renderer() ;
  // this is a no-op for x11, no options are handled
  //  void HandleOptions(int &argc, char *argv[]);
  int ComputeShift(unsigned long mask);

  void Render(int frameNumber,
              const Rectangle *imageRegion,
              int destX, int destY, float zoom, int lod);
  
  // from X11RendererGlue
  Display *display;
  Visual *visual;
  Drawable drawable;
  unsigned int depth;
  int doubleBuffered;
  int fontHeight;
  // was global: 
  XdbeSwapAction mSwapAction;
  // from WindowInfo struct in xwindow.cpp
  GC gc;
  XdbeBackBuffer backBuffer;

} ;

#endif
