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
  x11Renderer(ProgramOptions *options, qint32 parentWindowID, BlockbusterInterface *gui = NULL): 
    Renderer(options, parentWindowID, gui), mSwapAction(XdbeBackground) {
    mName = "x11";
    return; 
  }
    
    virtual ~x11Renderer() {
      XFreeGC(mDisplay, mGC);
      
      return; 
    }

  
  // ======================================================================
    virtual void BeginRendererInit(void) {
      return; 
    }

  // ======================================================================
  virtual void FinishRendererInit(void);

  int ComputeShift(unsigned long mask);
  void ChooseVisual(void) {
    return ; // does not apply to X11
  }
 
  ImagePtr ScaleImage(ImagePtr image, 
                      int srcX, int srcY, int srcWidth, int srcHeight,
                      int zoomedWidth, int zoomedHeight);
  virtual void RenderActual(Rectangle ROI);
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
